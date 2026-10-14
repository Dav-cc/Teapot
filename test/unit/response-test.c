#define _GNU_SOURCE

#include <criterion/criterion.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/http/http_response.h"
#include "../../src/core/dbuff.h"

#define HEADER_LIMIT 32

typedef struct {
    http_response *res;
    dbuffer *out;
} response_fixture;

static void fixture_init(response_fixture *f, size_t initial_capacity)
{
    f->res = http_response_create();
    f->out = dbuff_create(initial_capacity);

    cr_assert_not_null(f->res);
    cr_assert_not_null(f->out);
}

static void fixture_destroy(response_fixture *f)
{
    if (f->res != NULL) {
        http_response_reset(f->res);
        f->res = NULL;
    }

    if (f->out != NULL) {
        dbuff_destroy(f->out);
        f->out = NULL;
    }
}

static void set_status_and_reason(http_response *res, status code)
{
    http_response_set_status(res, code);
    http_response_set_reason(res);
}

static void expect_slice(const http_slice *slice,
                         const char *expected,
                         size_t expected_len)
{
    cr_assert_eq(slice->len, expected_len);

    if (expected_len > 0) {
        cr_assert_not_null(slice->ptr);
        cr_assert_eq(memcmp(slice->ptr, expected, expected_len), 0);
    }
}

static size_t count_bytes(const char *data,
                          size_t data_len,
                          const char *needle,
                          size_t needle_len)
{
    size_t count = 0;

    if (data == NULL || needle == NULL || needle_len == 0)
        return 0;

    if (needle_len > data_len)
        return 0;

    for (size_t i = 0; i <= data_len - needle_len; i++) {
        if (memcmp(data + i, needle, needle_len) == 0)
            count++;
    }

    return count;
}

static int contains_bytes(const char *data,
                          size_t data_len,
                          const char *needle,
                          size_t needle_len)
{
    return count_bytes(data, data_len, needle, needle_len) > 0;
}

static void expect_wire(response_fixture *f,
                        const char *expected,
                        size_t expected_len)
{
    cr_assert_eq((size_t)f->out->len, expected_len);

    if (expected_len > 0)
        cr_assert_eq(memcmp(f->out->data, expected, expected_len), 0);
}

/* ============================================================
 * Object creation and initial state
 * ============================================================ */

Test(response_create, creates_zero_initialized_response)
{
    http_response *res = http_response_create();

    cr_assert_not_null(res);
    cr_assert_eq(res->code, 0);
    cr_assert_eq(res->headers_count, 0);
    cr_assert_eq(res->body.ptr, NULL);
    cr_assert_eq(res->body.len, 0);
    cr_assert_eq(res->reason.ptr, NULL);
    cr_assert_eq(res->reason.len, 0);
    cr_assert_eq(res->con_len, 0);

    http_response_reset(res);
}

Test(response_create, can_create_multiple_independent_responses)
{
    http_response *a = http_response_create();
    http_response *b = http_response_create();

    cr_assert_not_null(a);
    cr_assert_not_null(b);
    cr_assert_neq(a, b);

    http_response_set_status(a, ST_OK);

    cr_assert_eq(a->code, ST_OK);
    cr_assert_eq(b->code, 0);

    http_response_reset(a);
    http_response_reset(b);
}

/* ============================================================
 * Status code
 * ============================================================ */

Test(response_status, sets_200_ok)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);

    cr_assert_eq(f.res->code, 200);
    expect_slice(&f.res->reason, "OK", 2);

    fixture_destroy(&f);
}

Test(response_status, sets_400_bad_request)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_BAD_REQUEST);

    cr_assert_eq(f.res->code, 400);
    expect_slice(&f.res->reason, "Bad Request", 11);

    fixture_destroy(&f);
}

Test(response_status, sets_404_not_found)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_NOT_FOUND);

    cr_assert_eq(f.res->code, 404);
    expect_slice(&f.res->reason, "Not Found", 9);

    fixture_destroy(&f);
}

Test(response_status, sets_418_teapot)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_TEAPOT);

    cr_assert_eq(f.res->code, 418);
    expect_slice(&f.res->reason, "I'm a teapot", 12);

    fixture_destroy(&f);
}

Test(response_status, sets_500_internal_server_error)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_INTERNAL_SERVER_ERROR);

    cr_assert_eq(f.res->code, 500);
    expect_slice(&f.res->reason, "Internal Server Error", 21);

    fixture_destroy(&f);
}

Test(response_status, sets_501_not_implemented)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_NOT_IMPLEMENTED);

    cr_assert_eq(f.res->code, 501);
    expect_slice(&f.res->reason, "Not Implemented", 15);

    fixture_destroy(&f);
}

Test(response_status, updates_reason_after_status_changes)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    set_status_and_reason(f.res, ST_NOT_FOUND);

    cr_assert_eq(f.res->code, 404);
    expect_slice(&f.res->reason, "Not Found", 9);

    fixture_destroy(&f);
}

/* ============================================================
 * Headers
 * ============================================================ */

Test(response_headers, adds_one_header)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_add_header(f.res, "Server", "Teapot");

    cr_assert_eq(f.res->headers_count, 1);
    expect_slice(&f.res->headers[0].name, "Server", 6);
    expect_slice(&f.res->headers[0].value, "Teapot", 6);

    fixture_destroy(&f);
}

Test(response_headers, adds_multiple_headers_in_order)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_add_header(f.res, "Server", "Teapot");
    http_response_add_header(f.res, "Content-Type", "text/plain");
    http_response_add_header(f.res, "Connection", "close");

    cr_assert_eq(f.res->headers_count, 3);

    expect_slice(&f.res->headers[0].name, "Server", 6);
    expect_slice(&f.res->headers[1].name, "Content-Type", 12);
    expect_slice(&f.res->headers[2].name, "Connection", 10);

    fixture_destroy(&f);
}

Test(response_headers, accepts_empty_header_value)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_add_header(f.res, "X-Empty", "");

    cr_assert_eq(f.res->headers_count, 1);
    expect_slice(&f.res->headers[0].name, "X-Empty", 7);
    cr_assert_eq(f.res->headers[0].value.len, 0);

    fixture_destroy(&f);
}

Test(response_headers, accepts_empty_body_independently_of_headers)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_add_header(f.res, "X-Test", "value");

    cr_assert_eq(f.res->headers_count, 1);
    cr_assert_eq(f.res->body.len, 0);

    fixture_destroy(&f);
}

Test(response_headers, does_not_exceed_header_limit)
{
    response_fixture f;
    fixture_init(&f, 128);

    char names[HEADER_LIMIT][16];

    for (size_t i = 0; i < HEADER_LIMIT; i++) {
        snprintf(names[i], sizeof(names[i]), "X-Test-%zu", i);
        http_response_add_header(f.res, names[i], "value");
    }

    cr_assert_eq(f.res->headers_count, HEADER_LIMIT);

    http_response_add_header(f.res, "X-Overflow", "value");

    cr_assert_eq(f.res->headers_count, HEADER_LIMIT);

    fixture_destroy(&f);
}

/* ============================================================
 * Body
 * ============================================================ */

Test(response_body, sets_normal_body)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_set_body(f.res, "hello");

    expect_slice(&f.res->body, "hello", 5);

    fixture_destroy(&f);
}

Test(response_body, sets_empty_body)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_set_body(f.res, "");

    cr_assert_eq(f.res->body.len, 0);

    fixture_destroy(&f);
}

Test(response_body, replacing_body_updates_pointer_and_length)
{
    response_fixture f;
    fixture_init(&f, 128);

    http_response_set_body(f.res, "first");
    http_response_set_body(f.res, "a longer body");

    expect_slice(&f.res->body, "a longer body", 13);

    fixture_destroy(&f);
}

Test(response_body, serializer_preserves_embedded_nul_bytes)
{
    response_fixture f;
    fixture_init(&f, 128);

    const char binary_body[] = { 'A', '\0', 'B', '\n' };

    set_status_and_reason(f.res, ST_OK);

    /*
     * The current set_body API uses strlen(), so binary payloads
     * are installed as an explicit length-aware slice here.
     */
    f.res->body.ptr = (char *)binary_body;
    f.res->body.len = sizeof(binary_body);
    f.res->con_len = f.res->body.len;

    size_t serialized = http_response_serializer(f.res, f.out);

    cr_assert_gt(serialized, 0);
    cr_assert_eq((size_t)f.out->len, serialized);

    const char separator[] = "\r\n\r\n";
    char *body_start = NULL;

    for (size_t i = 0; i + sizeof(separator) - 1 <= serialized; i++) {
        if (memcmp(f.out->data + i,
                   separator,
                   sizeof(separator) - 1) == 0) {
            body_start = f.out->data + i + sizeof(separator) - 1;
            break;
        }
    }

    cr_assert_not_null(body_start);
    cr_assert_eq(
        memcmp(body_start, binary_body, sizeof(binary_body)), 0
    );

    cr_assert_eq(
        serialized - (size_t)(body_start - f.out->data),
        sizeof(binary_body)
    );

    fixture_destroy(&f);
}

/* ============================================================
 * Serialization: status line and headers
 * ============================================================ */

Test(response_serializer, serializes_status_line)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "");

    http_response_serializer(f.res, f.out);

    const char expected[] = "HTTP/1.1 200 OK\r\n";

    cr_assert((size_t)f.out->len >= sizeof(expected) - 1);
    cr_assert_eq(
        memcmp(f.out->data, expected, sizeof(expected) - 1), 0
    );

    fixture_destroy(&f);
}

Test(response_serializer, serializes_header_format)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_add_header(f.res, "Content-Type", "text/plain");
    http_response_set_body(f.res, "");

    http_response_serializer(f.res, f.out);

    cr_assert(contains_bytes(
        f.out->data, (size_t)f.out->len,
        "Content-Type: text/plain\r\n",
        strlen("Content-Type: text/plain\r\n")
    ));

    fixture_destroy(&f);
}

Test(response_serializer, terminates_headers_with_empty_line)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "");

    http_response_serializer(f.res, f.out);

    cr_assert(contains_bytes(
        f.out->data, (size_t)f.out->len,
        "\r\n\r\n", 4
    ));

    fixture_destroy(&f);
}

Test(response_serializer, serializes_complete_simple_response)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_add_header(f.res, "Server", "Teapot");
    http_response_set_body(f.res, "hello");

    http_response_serializer(f.res, f.out);

    const char expected[] =
        "HTTP/1.1 200 OK\r\n"
        "Server: Teapot\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

    expect_wire(&f, expected, sizeof(expected) - 1);

    fixture_destroy(&f);
}

Test(response_serializer, generates_zero_content_length_for_empty_body)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "");

    http_response_serializer(f.res, f.out);

    cr_assert(contains_bytes(
        f.out->data, (size_t)f.out->len,
        "Content-Length: 0\r\n",
        strlen("Content-Length: 0\r\n")
    ));

    fixture_destroy(&f);
}

Test(response_serializer, does_not_emit_duplicate_content_length)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "hello");

    http_response_serializer(f.res, f.out);

    size_t count = count_bytes(
        f.out->data, (size_t)f.out->len,
        "Content-Length:", strlen("Content-Length:")
    );

    cr_assert_eq(count, 1);

    fixture_destroy(&f);
}


/* ============================================================
 * Serialization: buffer behavior
 * ============================================================ */

Test(response_serializer, appends_to_existing_output_buffer)
{
    response_fixture f;
    fixture_init(&f, 128);

    const char prefix[] = "PREFIX:";
    cr_assert_eq(
        dbuff_append(f.out, (char *)prefix, sizeof(prefix) - 1),
        IO_DONE
    );

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "hello");

    size_t result = http_response_serializer(f.res, f.out);

    cr_assert_eq(memcmp(f.out->data, prefix, sizeof(prefix) - 1), 0);
    cr_assert_eq(result, (size_t)f.out->len);
    cr_assert_gt(result, sizeof(prefix) - 1);

    const char expected_start[] = "PREFIX:HTTP/1.1 200 OK\r\n";

    cr_assert_eq(
        memcmp(f.out->data, expected_start, sizeof(expected_start) - 1),
        0
    );

    fixture_destroy(&f);
}

Test(response_serializer, returns_total_output_length)
{
    response_fixture f;
    fixture_init(&f, 128);

    set_status_and_reason(f.res, ST_OK);
    http_response_set_body(f.res, "hello");

    size_t result = http_response_serializer(f.res, f.out);

    cr_assert_eq(result, (size_t)f.out->len);

    fixture_destroy(&f);
}

Test(response_serializer, grows_small_output_buffer)
{
    response_fixture f;
    fixture_init(&f, 8);

    set_status_and_reason(f.res, ST_OK);
    http_response_add_header(f.res, "Content-Type", "text/plain");
    http_response_set_body(f.res, "hello");

    size_t result = http_response_serializer(f.res, f.out);

    cr_assert_gt(result, 8);
    cr_assert_eq(result, (size_t)f.out->len);
    cr_assert(contains_bytes(
        f.out->data, (size_t)f.out->len,
        "hello", 5
    ));

    fixture_destroy(&f);
}

Test(response_serializer, serializes_large_body)
{
    response_fixture f;
    fixture_init(&f, 128);

    const size_t body_len = 60000;
    char *body = malloc(body_len);

    cr_assert_not_null(body);
    memset(body, 'A', body_len);

    set_status_and_reason(f.res, ST_OK);
    f.res->body.ptr = body;
    f.res->body.len = body_len;
    f.res->con_len = body_len;

    size_t result = http_response_serializer(f.res, f.out);

    cr_assert_gt(result, body_len);
    cr_assert_eq(result, (size_t)f.out->len);

    cr_assert(contains_bytes(
        f.out->data, (size_t)f.out->len,
        "Content-Length: 60000\r\n",
        strlen("Content-Length: 60000\r\n")
    ));

    const char separator[] = "\r\n\r\n";
    char *body_start = NULL;

    for (size_t i = 0; i + 4 <= result; i++) {
        if (memcmp(f.out->data + i, separator, 4) == 0) {
            body_start = f.out->data + i + 4;
            break;
        }
    }

    cr_assert_not_null(body_start);
    cr_assert_eq(result - (size_t)(body_start - f.out->data), body_len);

    for (size_t i = 0; i < body_len; i++)
        cr_assert_eq(body_start[i], 'A');

    free(body);
    fixture_destroy(&f);
}

/* ============================================================
 * Repeated serialization and reset
 * ============================================================ */

Test(response_reset, clears_response_before_releasing_it)
{
    http_response *res = http_response_create();

    cr_assert_not_null(res);

    http_response_set_status(res, ST_OK);
    http_response_add_header(res, "Server", "Teapot");
    http_response_set_body(res, "hello");

    /*
     * Current API implementation also frees res.
     * Do not access this pointer after calling reset.
     */
    http_response_reset(res);
}
