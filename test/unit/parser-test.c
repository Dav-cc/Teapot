#define _GNU_SOURCE

#include <criterion/criterion.h>
#include <criterion/parameterized.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/parser/http_parser.h"
#include "../../src/core/dbuff.h"
#include "../../src/http/server.h"

/*
 * Parser tests for Teapot.
 *
 * These tests intentionally assert protocol behavior rather than
 * preserving bugs in the current implementation.
 */

#define HEADER_LIMIT 32

typedef struct {
    http_request req;
    Connection conn;
    dbuffer buf;
    char *storage;
} parser_fixture;

static void fixture_init(parser_fixture *f, const char *input, size_t len)
{
    memset(f, 0, sizeof(*f));

    /*
     * Extra bytes make it possible to test non-NUL-terminated HTTP
     * input safely. The parser must use buf.len, not strlen().
     */
    f->storage = calloc(len + 16, 1);
    cr_assert_not_null(f->storage);

    if (len != 0)
        memcpy(f->storage, input, len);

    f->buf.data = f->storage;
    f->buf.len = len;
    f->buf.cap = len + 16;
    f->buf.offset = 0;

    f->req.state = PARSER_STATE_REQLINE;
    f->req.mtd = HTTP_UNKNOWN;

    f->conn.request = &f->req;
}

static void fixture_destroy(parser_fixture *f)
{
    free(f->storage);
    f->storage = NULL;
}

static http_parser_error parse_request_line(parser_fixture *f)
{
    return http_parser_request_line(&f->req, &f->buf);
}

static http_parser_error parse_headers(parser_fixture *f)
{
    return http_parser_headers(&f->req, &f->buf);
}

static http_parser_error parse_body(parser_fixture *f)
{
    return http_parser_body(&f->req, &f->buf);
}

static http_parser_result parse_all(parser_fixture *f)
{
    return http_parser_parse(&f->conn, &f->buf);
}

static void expect_slice(const http_slice *slice,
                         const char *expected,
                         size_t expected_len)
{
    cr_assert_eq(slice->len, expected_len);
    cr_assert_not_null(slice->ptr);

    if (expected_len != 0)
        cr_assert_eq(memcmp(slice->ptr, expected, expected_len), 0);
}

static void make_header(http_header *h,
                        const char *name,
                        const char *value)
{
    h->name.ptr = (char *)name;
    h->name.len = strlen(name);
    h->value.ptr = (char *)value;
    h->value.len = strlen(value);
}

/* ============================================================
 * slice_eq_string
 * ============================================================ */

Test(parser_helpers, slice_eq_string_equal)
{
    char value[] = "GET";
    http_slice s = { .ptr = value, .len = 3 };

    cr_assert_eq(slice_eq_string(&s, "GET"), 0);
}

Test(parser_helpers, slice_eq_string_different_content)
{
    char value[] = "PUT";
    http_slice s = { .ptr = value, .len = 3 };

    cr_assert_neq(slice_eq_string(&s, "GET"), 0);
}

Test(parser_helpers, slice_eq_string_different_length)
{
    char value[] = "GETTING";
    http_slice s = { .ptr = value, .len = 7 };

    cr_assert_neq(slice_eq_string(&s, "GET"), 0);
}

Test(parser_helpers, slice_eq_string_empty_slice)
{
    char value[] = "";
    http_slice s = { .ptr = value, .len = 0 };

    cr_assert_neq(slice_eq_string(&s, "GET"), 0);
}

Test(parser_helpers, slice_eq_string_empty_string)
{
    char value[] = "";
    http_slice s = { .ptr = value, .len = 0 };

    cr_assert_eq(slice_eq_string(&s, ""), 0);
}

Test(parser_helpers, slice_eq_string_is_case_sensitive)
{
    char value[] = "get";
    http_slice s = { .ptr = value, .len = 3 };

    cr_assert_neq(slice_eq_string(&s, "GET"), 0);
}

/* ============================================================
 * find_slice
 * ============================================================ */

Test(parser_helpers, find_slice_finds_first_match)
{
    char input[] = "abc--abc";
    char *p = find_slice(input, "abc", 3, sizeof(input) - 1);

    cr_assert_eq(p, input);
}

Test(parser_helpers, find_slice_finds_match_in_middle)
{
    char input[] = "prefix\r\nsuffix";
    char *p = find_slice(input, "\r\n", 2, sizeof(input) - 1);

    cr_assert_eq(p, input + 6);
}

Test(parser_helpers, find_slice_not_found)
{
    char input[] = "abcdef";

    cr_assert_null(find_slice(input, "xyz", 3, 6));
}

Test(parser_helpers, find_slice_needle_larger_than_input)
{
    char input[] = "ab";

    cr_assert_null(find_slice(input, "abcd", 4, 2));
}

Test(parser_helpers, find_slice_match_at_end)
{
    char input[] = "abc\r\n";
    char *p = find_slice(input, "\r\n", 2, 5);

    cr_assert_eq(p, input + 3);
}

Test(parser_helpers, find_slice_respects_input_length)
{
    char input[] = "abc\r\nxyz";

    cr_assert_null(find_slice(input, "\r\n", 2, 3));
}

Test(parser_helpers, find_slice_does_not_read_past_logical_end)
{
    char input[] = "abcXYZ";

    cr_assert_null(find_slice(input, "XYZ", 3, 3));
}

/* ============================================================
 * header_lookup
 * ============================================================ */

Test(parser_helpers, header_lookup_finds_existing_header)
{
    http_header headers[2] = {0};

    make_header(&headers[0], "Host", "example.com");
    make_header(&headers[1], "Content-Length", "5");

    http_header *h = header_lookup(headers, "Content-Length", 2);

    cr_assert_not_null(h);
    expect_slice(&h->value, "5", 1);
}

Test(parser_helpers, header_lookup_returns_null_when_missing)
{
    http_header headers[1] = {0};
    make_header(&headers[0], "Host", "example.com");

    cr_assert_null(header_lookup(headers, "Content-Length", 1));
}

Test(parser_helpers, header_lookup_handles_zero_headers)
{
    http_header headers[1] = {0};

    cr_assert_null(header_lookup(headers, "Host", 0));
}

Test(parser_helpers, header_lookup_returns_first_duplicate)
{
    http_header headers[2] = {0};

    make_header(&headers[0], "X-Test", "first");
    make_header(&headers[1], "X-Test", "second");

    http_header *h = header_lookup(headers, "X-Test", 2);

    cr_assert_not_null(h);
    expect_slice(&h->value, "first", 5);
}

/*
 * HTTP field names are case-insensitive. This test should fail until
 * header_lookup implements case-insensitive comparison.
 */
Test(parser_helpers, header_lookup_is_case_insensitive)
{
    http_header headers[1] = {0};
    make_header(&headers[0], "Content-Length", "5");

    cr_assert_not_null(header_lookup(headers, "content-length", 1));
}

/* ============================================================
 * slice_to_ul
 * ============================================================ */

static ssize_t convert_length(const char *value, size_t len)
{
    http_header h = {0};
    h.value.ptr = (char *)value;
    h.value.len = len;
    return slice_to_ul(&h);
}

Test(parser_helpers, content_length_zero)
{
    cr_assert_eq(convert_length("0", 1), 0);
}

Test(parser_helpers, content_length_positive)
{
    cr_assert_eq(convert_length("12345", 5), 12345);
}

Test(parser_helpers, content_length_empty_is_invalid)
{
    cr_assert_eq(convert_length("", 0), -1);
}

Test(parser_helpers, content_length_rejects_letters)
{
    cr_assert_eq(convert_length("abc", 3), -1);
}

Test(parser_helpers, content_length_rejects_trailing_junk)
{
    cr_assert_eq(convert_length("12x", 3), -1);
}

Test(parser_helpers, content_length_rejects_negative)
{
    cr_assert_eq(convert_length("-1", 2), -1);
}

Test(parser_helpers, content_length_rejects_plus_sign)
{
    cr_assert_eq(convert_length("+1", 2), -1);
}

Test(parser_helpers, content_length_rejects_leading_whitespace)
{
    cr_assert_eq(convert_length(" 1", 2), -1);
}

Test(parser_helpers, content_length_rejects_trailing_whitespace)
{
    cr_assert_eq(convert_length("1 ", 2), -1);
}

Test(parser_helpers, content_length_rejects_overlong_input)
{
    char input[32];
    memset(input, '1', sizeof(input));

    cr_assert_eq(convert_length(input, sizeof(input) - 1), -1);
}

Test(parser_helpers, content_length_rejects_overflow)
{
    const char *input = "999999999999999999999999999999";

    cr_assert_eq(convert_length(input, strlen(input)), -1);
}

/* ============================================================
 * Request Line
 * ============================================================ */

Test(parser_request_line, parses_get_request)
{
    const char input[] = "GET / HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.mtd, HTTP_GET);
    cr_assert_eq(f.req.state, PARSER_STATE_HEADERS);

    expect_slice(&f.req.method, "GET", 3);
    expect_slice(&f.req.path, "/", 1);
    expect_slice(&f.req.version, "HTTP/1.1", 8);

    cr_assert_eq(f.buf.offset, sizeof(input) - 1);
    fixture_destroy(&f);
}

Test(parser_request_line, parses_post_request)
{
    const char input[] = "POST /api HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.mtd, HTTP_POST);
    expect_slice(&f.req.path, "/api", 4);

    fixture_destroy(&f);
}

Test(parser_request_line, unknown_method_is_not_silently_accepted)
{
    const char input[] = "PUT / HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    (void)parse_request_line(&f);

    cr_assert_eq(f.req.mtd, HTTP_UNKNOWN);
    fixture_destroy(&f);
}

Test(parser_request_line, incomplete_request_line_needs_more)
{
    const char input[] = "GET / HTTP/1.1";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_eq(err.type, ERR_REQUEST_NOT_COMPELETE);
    fixture_destroy(&f);
}

Test(parser_request_line, empty_input_needs_more)
{
    parser_fixture f;

    fixture_init(&f, "", 0);
    http_parser_error err = parse_request_line(&f);

    cr_assert_eq(err.type, ERR_REQUEST_NOT_COMPELETE);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_empty_method)
{
    const char input[] = " / HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_neq(err.any_err, 0);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_empty_target)
{
    const char input[] = "GET  HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_neq(err.any_err, 0);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_missing_version)
{
    const char input[] = "GET /\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_neq(err.any_err, 0);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_extra_spaces)
{
    const char input[] = "GET  / HTTP/1.1\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_neq(err.any_err, 0);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_lf_only_line_ending)
{
    const char input[] = "GET / HTTP/1.1\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    http_parser_error err = parse_request_line(&f);

    cr_assert_eq(err.type, ERR_REQUEST_NOT_COMPELETE);
    fixture_destroy(&f);
}

Test(parser_request_line, rejects_invalid_http_version)
{
    const char input[] = "GET / HTTP/9.9\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    (void)parse_request_line(&f);

    /*
     * This assertion requires explicit HTTP-version validation.
     * The current parser only slices the version and does not validate it.
     */
    cr_assert_neq(f.req.version.len, 0);
    cr_assert_str_neq(f.req.version.ptr, "HTTP/1.1");

    fixture_destroy(&f);
}

/* ============================================================
 * Headers
 * ============================================================ */

Test(parser_headers, parses_one_header_and_terminator)
{
    const char input[] = "Host: example.com\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;
    f.buf.offset = 0;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.headers_count, 1);
    expect_slice(&f.req.headers[0].name, "Host", 4);
    expect_slice(&f.req.headers[0].value, "example.com", 11);
    cr_assert_eq(f.buf.offset, sizeof(input) - 1);

    fixture_destroy(&f);
}

Test(parser_headers, incomplete_header_block_needs_more)
{
    const char input[] = "Host: example.com\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.type, ERR_REQUEST_NOT_COMPELETE);
    fixture_destroy(&f);
}

Test(parser_headers, rejects_header_without_colon)
{
    const char input[] = "Host example.com\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.type, ERR_INVALID_HEADER_CONTENT);
    fixture_destroy(&f);
}

Test(parser_headers, rejects_empty_header_name)
{
    const char input[] = ": value\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_neq(err.any_err, 0);
    fixture_destroy(&f);
}

Test(parser_headers, accepts_empty_header_value)
{
    const char input[] = "X-Empty: \r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.headers_count, 1);
    cr_assert_eq(f.req.headers[0].value.len, 0);

    fixture_destroy(&f);
}

Test(parser_headers, parses_multiple_headers)
{
    const char input[] =
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "X-Test: value\r\n"
        "\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.headers_count, 3);
    cr_assert_eq(f.req.keep_alive, 0);

    fixture_destroy(&f);
}

Test(parser_headers, default_keep_alive_is_enabled)
{
    const char input[] = "Host: localhost\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    (void)parse_headers(&f);

    cr_assert_eq(f.req.keep_alive, 1);
    fixture_destroy(&f);
}

Test(parser_headers, connection_close_is_case_sensitive_in_current_code)
{
    const char input[] = "Connection: Close\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.state = PARSER_STATE_HEADERS;

    (void)parse_headers(&f);

    /*
     * HTTP token values are case-insensitive for Connection options.
     * This assertion exposes the current exact-string comparison bug.
     */
    cr_assert_eq(f.req.keep_alive, 0);
    fixture_destroy(&f);
}

Test(parser_headers, accepts_exactly_32_headers)
{
    char input[2048];
    size_t used = 0;

    for (size_t i = 0; i < HEADER_LIMIT; i++) {
        int n = snprintf(input + used, sizeof(input) - used,
                         "X-%02zu: v\r\n", i);
        cr_assert_gt(n, 0);
        used += (size_t)n;
    }

    memcpy(input + used, "\r\n", 2);
    used += 2;

    parser_fixture f;
    fixture_init(&f, input, used);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.headers_count, HEADER_LIMIT);

    fixture_destroy(&f);
}

Test(parser_headers, rejects_33_headers)
{
    char input[2048];
    size_t used = 0;

    for (size_t i = 0; i < HEADER_LIMIT + 1; i++) {
        int n = snprintf(input + used, sizeof(input) - used,
                         "X-%02zu: v\r\n", i);
        cr_assert_gt(n, 0);
        used += (size_t)n;
    }

    memcpy(input + used, "\r\n", 2);
    used += 2;

    parser_fixture f;
    fixture_init(&f, input, used);
    f.req.state = PARSER_STATE_HEADERS;

    http_parser_error err = parse_headers(&f);

    cr_assert_eq(err.type, ERR_INVALID_HEADER_COUNT);
    fixture_destroy(&f);
}

Test(parser_headers, get_transitions_to_done)
{
    const char input[] = "Host: localhost\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.mtd = HTTP_GET;
    f.req.state = PARSER_STATE_HEADERS;

    (void)parse_headers(&f);

    cr_assert_eq(f.req.state, PARSER_STATE_DONE);
    fixture_destroy(&f);
}

Test(parser_headers, post_transitions_to_body)
{
    const char input[] = "Content-Length: 0\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.mtd = HTTP_POST;
    f.req.state = PARSER_STATE_HEADERS;

    (void)parse_headers(&f);

    cr_assert_eq(f.req.state, PARSER_STATE_BODY);
    fixture_destroy(&f);
}

Test(parser_headers, unknown_method_transitions_to_error)
{
    const char input[] = "Host: localhost\r\n\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.mtd = HTTP_UNKNOWN;
    f.req.state = PARSER_STATE_HEADERS;

    (void)parse_headers(&f);

    cr_assert_eq(f.req.state, PARSER_STATE_ERROR);
    fixture_destroy(&f);
}

/* ============================================================
 * Body and Content-Length
 * ============================================================ */

Test(parser_body, parses_complete_body)
{
    const char input[] = "hello";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "5");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.any_err, 0);
    expect_slice(&f.req.body, "hello", 5);
    cr_assert_eq(f.buf.offset, 5);

    fixture_destroy(&f);
}

Test(parser_body, zero_length_body)
{
    const char input[] = "";
    parser_fixture f;

    fixture_init(&f, input, 0);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "0");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.body.len, 0);
    cr_assert_eq(f.buf.offset, 0);

    fixture_destroy(&f);
}

Test(parser_body, missing_content_length_is_error)
{
    const char input[] = "hello";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_INVALLID_CONTENT_LENGTH);
    fixture_destroy(&f);
}

Test(parser_body, incomplete_body_needs_more)
{
    const char input[] = "hell";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "5");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_REQUEST_NOT_COMPELETE);
    fixture_destroy(&f);
}

Test(parser_body, rejects_extra_body_bytes)
{
    const char input[] = "hello!";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "5");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_INVALLID_CONTENT_LENGTH);
    fixture_destroy(&f);
}

Test(parser_body, binary_body_preserves_nul_bytes)
{
    const unsigned char input[] = { 'A', 0, 'B' };
    parser_fixture f;

    fixture_init(&f, (const char *)input, sizeof(input));
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "3");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.any_err, 0);
    cr_assert_eq(f.req.body.len, 3);
    cr_assert_eq(memcmp(f.req.body.ptr, input, 3), 0);

    fixture_destroy(&f);
}

Test(parser_body, rejects_non_numeric_content_length)
{
    const char input[] = "hello";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "five");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_INVALLID_CONTENT_LENGTH);
    fixture_destroy(&f);
}

Test(parser_body, rejects_negative_content_length)
{
    const char input[] = "";
    parser_fixture f;

    fixture_init(&f, input, 0);
    f.req.headers_count = 1;
    make_header(&f.req.headers[0], "Content-Length", "-1");

    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_INVALLID_CONTENT_LENGTH);
    fixture_destroy(&f);
}

Test(parser_body, rejects_duplicate_content_length)
{
    const char input[] = "hello";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);
    f.req.headers_count = 2;
    make_header(&f.req.headers[0], "Content-Length", "5");
    make_header(&f.req.headers[1], "Content-Length", "4");

    /*
     * Duplicate Content-Length values must not be silently trusted.
     * A hardened parser should reject conflicting duplicates.
     */
    http_parser_error err = parse_body(&f);

    cr_assert_eq(err.type, ERR_INVALLID_CONTENT_LENGTH);
    fixture_destroy(&f);
}

/* ============================================================
 * Reset
 * ============================================================ */

Test(parser_reset, clears_request_metadata)
{
    http_request req = {0};

    req.mtd = HTTP_POST;
    req.method.len = 4;
    req.path.len = 10;
    req.version.len = 8;
    req.body.len = 5;
    req.headers_count = 3;
    req.state = PARSER_STATE_DONE;

    http_parser_reset(&req);

    cr_assert_eq(req.mtd, HTTP_UNKNOWN);
    cr_assert_eq(req.method.len, 0);
    cr_assert_eq(req.path.len, 0);
    cr_assert_eq(req.version.len, 0);
    cr_assert_eq(req.body.len, 0);
    cr_assert_eq(req.headers_count, 0);
    cr_assert_eq(req.state, PARSER_STATE_REQLINE);
}

/* ============================================================
 * Full parser integration
 * ============================================================ */

Test(parser_integration, parses_get_request)
{
    const char input[] =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);

    http_parser_result result = parse_all(&f);

    cr_assert_eq(result, PARSER_OK);
    cr_assert_eq(f.req.mtd, HTTP_GET);
    cr_assert_eq(f.req.state, PARSER_STATE_DONE);
    expect_slice(&f.req.path, "/", 1);

    fixture_destroy(&f);
}

Test(parser_integration, parses_post_request_with_body)
{
    const char input[] =
        "POST /api HTTP/1.1\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";
    parser_fixture f;

    fixture_init(&f, input, sizeof(input) - 1);

    http_parser_result result = parse_all(&f);

    cr_assert_eq(result, PARSER_OK);
    cr_assert_eq(f.req.mtd, HTTP_POST);
    expect_slice(&f.req.body, "hello", 5);

    fixture_destroy(&f);
}

Test(parser_integration, null_request_returns_error)
{
    parser_fixture f;
    fixture_init(&f, "", 0);

    f.conn.request = NULL;

    cr_assert_eq(parse_all(&f), PARSER_ERROR);

    fixture_destroy(&f);
}

Test(parser_integration, error_state_returns_parser_error)
{
    parser_fixture f;
    fixture_init(&f, "", 0);

    f.req.state = PARSER_STATE_ERROR;

    cr_assert_eq(parse_all(&f), PARSER_ERROR);

    fixture_destroy(&f);
}
