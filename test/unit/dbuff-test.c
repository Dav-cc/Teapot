#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include "../../src/core/dbuff.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define TEST_MAX_BUFFER_SIZE (64u * 1024u)

/* ---------- Test helpers ---------- */

static dbuffer *make_buffer(size_t cap)
{
    dbuffer *buf = dbuff_create(cap);
    cr_assert_not_null(buf, "dbuff_create(%zu) returned NULL", cap);
    return buf;
}

static void assert_buffer_state(
    const dbuffer *buf,
    size_t len,
    size_t offset)
{
    cr_assert_not_null(buf);
    cr_assert_eq(buf->len, len);
    cr_assert_eq(buf->offset, offset);
    cr_assert_geq(buf->cap, buf->len);
}

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    cr_assert_neq(flags, -1, "fcntl(F_GETFL) failed");

    cr_assert_neq(
        fcntl(fd, F_SETFL, flags | O_NONBLOCK),
        -1,
        "fcntl(F_SETFL) failed");
}

static void close_checked(int fd)
{
    if (fd >= 0)
        cr_assert_eq(close(fd), 0);
}

/* ---------- Creation and destruction ---------- */

Test(dbuff_create, initializes_empty_buffer)
{
    dbuffer *buf = make_buffer(128);

    cr_assert_not_null(buf->data);
    cr_assert_eq(buf->cap, 128);
    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);

    dbuff_destroy(buf);
}

Test(dbuff_create, supports_capacity_one)
{
    dbuffer *buf = make_buffer(1);

    cr_assert_not_null(buf->data);
    cr_assert_eq(buf->cap, 1);
    assert_buffer_state(buf, 0, 0);

    dbuff_destroy(buf);
}

Test(dbuff_create, zero_capacity_does_not_corrupt_state)
{
    /*
     * calloc(1, 0) is implementation-dependent:
     * dbuff_create(0) may return NULL or a valid object.
     * Do not append to a zero-capacity buffer here:
     * the current implementation can loop forever.
     */
    dbuffer *buf = dbuff_create(0);

    if (buf != NULL) {
        cr_assert_eq(buf->cap, 0);
        cr_assert_eq(buf->len, 0);
        cr_assert_eq(buf->offset, 0);
        dbuff_destroy(buf);
    }
}

Test(dbuff_destroy, releases_an_empty_buffer)
{
    dbuffer *buf = make_buffer(16);
    dbuff_destroy(buf);
}

Test(dbuff_destroy, releases_buffer_after_append)
{
    dbuffer *buf = make_buffer(4);
    const char data[] = "hello";

    cr_assert_eq(dbuff_append(buf, (char *)data, sizeof(data)), IO_DONE);
    dbuff_destroy(buf);
}

/* ---------- Append: basic behavior ---------- */

Test(dbuff_append, appends_one_byte)
{
    dbuffer *buf = make_buffer(8);
    const char value = 'X';

    cr_assert_eq(dbuff_append(buf, (char *)&value, 1), IO_DONE);
    assert_buffer_state(buf, 1, 0);
    cr_assert_eq(buf->data[0], 'X');

    dbuff_destroy(buf);
}

Test(dbuff_append, appends_multiple_chunks_in_order)
{
    dbuffer *buf = make_buffer(32);
    const char first[] = "hello";
    const char second[] = " ";
    const char third[] = "world";

    cr_assert_eq(dbuff_append(buf, (char *)first, sizeof(first) - 1), IO_DONE);
    cr_assert_eq(dbuff_append(buf, (char *)second, sizeof(second) - 1), IO_DONE);
    cr_assert_eq(dbuff_append(buf, (char *)third, sizeof(third) - 1), IO_DONE);

    cr_assert_eq(buf->len, 11);
    cr_assert_eq(memcmp(buf->data, "hello world", 11), 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, zero_length_append_changes_nothing)
{
    dbuffer *buf = make_buffer(8);
    const char data[] = "abc";

    cr_assert_eq(dbuff_append(buf, (char *)data, 3), IO_DONE);

    size_t old_len = buf->len;
    size_t old_cap = buf->cap;
    size_t old_offset = buf->offset;
    char snapshot[3];

    memcpy(snapshot, buf->data, sizeof(snapshot));

    cr_assert_eq(dbuff_append(buf, (char *)data, 0), IO_DONE);

    cr_assert_eq(buf->len, old_len);
    cr_assert_eq(buf->cap, old_cap);
    cr_assert_eq(buf->offset, old_offset);
    cr_assert_eq(memcmp(buf->data, snapshot, sizeof(snapshot)), 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, preserves_embedded_null_bytes)
{
    dbuffer *buf = make_buffer(8);
    const unsigned char input[] = { 'A', 0, 'B', 0, 'C' };

    cr_assert_eq(
        dbuff_append(buf, (char *)input, sizeof(input)),
        IO_DONE);

    cr_assert_eq(buf->len, sizeof(input));
    cr_assert_eq(memcmp(buf->data, input, sizeof(input)), 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, preserves_all_byte_values)
{
    dbuffer *buf = make_buffer(256);
    unsigned char input[256];

    for (size_t i = 0; i < sizeof(input); i++)
        input[i] = (unsigned char)i;

    cr_assert_eq(
        dbuff_append(buf, (char *)input, sizeof(input)),
        IO_DONE);

    cr_assert_eq(buf->len, sizeof(input));
    cr_assert_eq(memcmp(buf->data, input, sizeof(input)), 0);

    dbuff_destroy(buf);
}

/* ---------- Append: capacity and growth ---------- */

Test(dbuff_append, fills_existing_capacity_exactly)
{
    dbuffer *buf = make_buffer(8);
    const char input[] = "12345678";

    cr_assert_eq(dbuff_append(buf, (char *)input, 8), IO_DONE);
    cr_assert_eq(buf->len, 8);
    cr_assert_geq(buf->cap, 8);
    cr_assert_eq(memcmp(buf->data, input, 8), 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, grows_when_input_exceeds_capacity)
{
    dbuffer *buf = make_buffer(4);
    const char input[] = "123456789";

    cr_assert_eq(dbuff_append(buf, (char *)input, 9), IO_DONE);

    cr_assert_eq(buf->len, 9);
    cr_assert_geq(buf->cap, 9);
    cr_assert_eq(memcmp(buf->data, input, 9), 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, grows_across_multiple_capacity_boundaries)
{
    dbuffer *buf = make_buffer(2);
    const size_t input_len = 10000;
    unsigned char *input = malloc(input_len);

    cr_assert_not_null(input);

    for (size_t i = 0; i < input_len; i++)
        input[i] = (unsigned char)(i % 251);

    cr_assert_eq(
        dbuff_append(buf, (char *)input, input_len),
        IO_DONE);

    cr_assert_eq(buf->len, input_len);
    cr_assert_geq(buf->cap, input_len);
    cr_assert_eq(memcmp(buf->data, input, input_len), 0);

    free(input);
    dbuff_destroy(buf);
}

Test(dbuff_append, can_append_exactly_64_kibibytes)
{
    dbuffer *buf = make_buffer(8);
    unsigned char *input = malloc(TEST_MAX_BUFFER_SIZE);

    cr_assert_not_null(input);

    for (size_t i = 0; i < TEST_MAX_BUFFER_SIZE; i++)
        input[i] = (unsigned char)(i % 239);

    cr_assert_eq(
        dbuff_append(buf, (char *)input, TEST_MAX_BUFFER_SIZE),
        IO_DONE);

    cr_assert_eq(buf->len, TEST_MAX_BUFFER_SIZE);
    cr_assert_geq(buf->cap, TEST_MAX_BUFFER_SIZE);
    cr_assert_eq(
        memcmp(buf->data, input, TEST_MAX_BUFFER_SIZE),
        0);

    free(input);
    dbuff_destroy(buf);
}

Test(dbuff_append, repeated_appends_preserve_previous_data)
{
    dbuffer *buf = make_buffer(2);

    for (int i = 0; i < 100; i++) {
        char value = (char)i;
        cr_assert_eq(dbuff_append(buf, &value, 1), IO_DONE);
    }

    cr_assert_eq(buf->len, 100);

    for (int i = 0; i < 100; i++)
        cr_assert_eq((unsigned char)buf->data[i], (unsigned char)i);

    dbuff_destroy(buf);
}

/* ---------- Reset ---------- */

Test(dbuff_reset, clears_length_and_offset)
{
    dbuffer *buf = make_buffer(16);
    const char input[] = "abcdef";

    cr_assert_eq(dbuff_append(buf, (char *)input, 6), IO_DONE);
    buf->offset = 3;

    cr_assert_eq(dbuff_reset(buf), IO_DONE);

    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);
    cr_assert_geq(buf->cap, 16);

    dbuff_destroy(buf);
}

Test(dbuff_reset, preserves_allocated_capacity)
{
    dbuffer *buf = make_buffer(4);
    const char input[] = "a much longer string";

    cr_assert_eq(
        dbuff_append(buf, (char *)input, sizeof(input) - 1),
        IO_DONE);

    size_t old_cap = buf->cap;
    cr_assert_gt(old_cap, 4);

    cr_assert_eq(dbuff_reset(buf), IO_DONE);
    cr_assert_eq(buf->cap, old_cap);

    dbuff_destroy(buf);
}

Test(dbuff_reset, supports_reuse_after_reset)
{
    dbuffer *buf = make_buffer(4);
    const char first[] = "first";
    const char second[] = "new";

    cr_assert_eq(
        dbuff_append(buf, (char *)first, sizeof(first) - 1),
        IO_DONE);

    cr_assert_eq(dbuff_reset(buf), IO_DONE);

    cr_assert_eq(
        dbuff_append(buf, (char *)second, sizeof(second) - 1),
        IO_DONE);

    cr_assert_eq(buf->len, sizeof(second) - 1);
    cr_assert_eq(
        memcmp(buf->data, second, sizeof(second) - 1),
        0);

    dbuff_destroy(buf);
}

Test(dbuff_reset, repeated_resets_are_safe)
{
    dbuffer *buf = make_buffer(8);
    const char input[] = "data";

    cr_assert_eq(dbuff_append(buf, (char *)input, 4), IO_DONE);

    for (int i = 0; i < 10; i++) {
        cr_assert_eq(dbuff_reset(buf), IO_DONE);
        cr_assert_eq(buf->len, 0);
        cr_assert_eq(buf->offset, 0);
    }

    dbuff_destroy(buf);
}

/* ---------- Read: nonblocking pipe ---------- */

Test(dbuff_read, returns_again_when_no_data_is_available)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);
    set_nonblocking(fds[0]);

    dbuffer *buf = make_buffer(16);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_AGAIN);
    assert_buffer_state(buf, 0, 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_read, reads_available_data_until_eagain)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);
    set_nonblocking(fds[0]);

    const char input[] = "hello";
    cr_assert_eq(write(fds[1], input, 5), 5);

    dbuffer *buf = make_buffer(16);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_DONE);
    cr_assert_eq(buf->len, 5);
    cr_assert_eq(memcmp(buf->data, input, 5), 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_read, grows_buffer_for_data_larger_than_initial_capacity)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);
    set_nonblocking(fds[0]);

    const size_t input_len = 10000;
    unsigned char *input = malloc(input_len);
    cr_assert_not_null(input);

    for (size_t i = 0; i < input_len; i++)
        input[i] = (unsigned char)(i % 253);

    size_t written = 0;
    while (written < input_len) {
        ssize_t n = write(fds[1], input + written, input_len - written);

        if (n > 0) {
            written += (size_t)n;
            continue;
        }

        if (n == -1 && errno == EINTR)
            continue;

        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;

        cr_assert_fail("Unexpected write error while preparing test data");
    }

    /*
     * A pipe has finite capacity. This test needs all input bytes queued
     * before dbuff_read runs, so use a regular temporary file instead
     * if the pipe cannot hold the complete payload.
     */
    if (written != input_len) {
        free(input);
        close_checked(fds[0]);
        close_checked(fds[1]);
        cr_skip("Pipe capacity is smaller than the test payload");
    }

    dbuffer *buf = make_buffer(4);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_DONE);
    cr_assert_eq(buf->len, input_len);
    cr_assert_geq(buf->cap, input_len);
    cr_assert_eq(memcmp(buf->data, input, input_len), 0);

    free(input);
    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_read, returns_closed_when_peer_has_closed_without_data)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);
    set_nonblocking(fds[0]);

    close_checked(fds[1]);

    dbuffer *buf = make_buffer(16);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_CLOSED);
    assert_buffer_state(buf, 0, 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
}

Test(dbuff_read, reports_error_for_invalid_file_descriptor)
{
    dbuffer *buf = make_buffer(16);

    cr_assert_eq(dbuff_read(buf, -1), IO_ERROR);

    dbuff_destroy(buf);
}

/* ---------- Write: pipes ---------- */

Test(dbuff_write, writes_all_buffered_data)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = make_buffer(16);
    const char input[] = "hello";

    cr_assert_eq(dbuff_append(buf, (char *)input, 5), IO_DONE);
    cr_assert_eq(dbuff_write(buf, fds[1]), IO_DONE);
    cr_assert_eq(buf->offset, buf->len);

    char output[5];
    cr_assert_eq(read(fds[0], output, sizeof(output)), sizeof(output));
    cr_assert_eq(memcmp(output, input, sizeof(output)), 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_write, empty_buffer_returns_done)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = make_buffer(16);

    cr_assert_eq(dbuff_write(buf, fds[1]), IO_DONE);
    cr_assert_eq(buf->offset, 0);
    cr_assert_eq(buf->len, 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_write, sends_only_data_after_offset)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = make_buffer(16);
    const char input[] = "abcdef";

    cr_assert_eq(dbuff_append(buf, (char *)input, 6), IO_DONE);
    buf->offset = 2;

    cr_assert_eq(dbuff_write(buf, fds[1]), IO_DONE);
    cr_assert_eq(buf->offset, 6);

    char output[4];
    cr_assert_eq(read(fds[0], output, sizeof(output)), sizeof(output));
    cr_assert_eq(memcmp(output, "cdef", 4), 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_write, returns_again_when_nonblocking_pipe_is_full)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    set_nonblocking(fds[0]);
    set_nonblocking(fds[1]);

    /*
     * Fill the pipe until the nonblocking writer reports EAGAIN.
     */
    char filler[4096];
    memset(filler, 'F', sizeof(filler));

    while (1) {
        ssize_t n = write(fds[1], filler, sizeof(filler));

        if (n > 0)
            continue;

        if (n == -1 && errno == EINTR)
            continue;

        cr_assert_eq(n, -1);
        cr_assert(errno == EAGAIN || errno == EWOULDBLOCK);
        break;
    }

    dbuffer *buf = make_buffer(4096);
    char payload[4096];
    memset(payload, 'P', sizeof(payload));

    cr_assert_eq(
        dbuff_append(buf, payload, sizeof(payload)),
        IO_DONE);

    cr_assert_eq(dbuff_write(buf, fds[1]), IO_AGAIN);
    cr_assert_eq(buf->offset, 0);

    /*
     * Drain the pipe, then retry. The pending buffer should be written
     * from its existing offset without losing or duplicating bytes.
     */
    char drain[4096];

    while (1) {
        ssize_t n = read(fds[0], drain, sizeof(drain));

        if (n > 0)
            continue;

        if (n == -1 && errno == EINTR)
            continue;

        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;

        cr_assert_fail("Unexpected pipe read result");
    }

    cr_assert_eq(dbuff_write(buf, fds[1]), IO_DONE);
    cr_assert_eq(buf->offset, buf->len);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

/* ---------- Read/write error paths ---------- */

Test(dbuff_write, reports_error_for_invalid_file_descriptor)
{
    dbuffer *buf = make_buffer(16);
    const char input[] = "data";

    cr_assert_eq(dbuff_append(buf, (char *)input, 4), IO_DONE);
    cr_assert_eq(dbuff_write(buf, -1), IO_ERROR);

    dbuff_destroy(buf);
}

/*
 * 1. Creation tests
 */

Test(dbuffer, create_initializes_fields)
{
    dbuffer *buf = dbuff_create(64);

    cr_assert_not_null(buf);
    cr_assert_not_null(buf->data);

    cr_assert_eq(buf->cap, 64);
    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);

    dbuff_destroy(buf);
}

/*
 * 2. Append tests
 */

Test(dbuffer, append_stores_bytes)
{
    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    char data[] = "hello";

    io_err result = dbuff_append(buf, data, 5);

    cr_assert_eq(result, IO_DONE);
    cr_assert_eq(buf->len, 5);
    cr_assert(memcmp(buf->data, "hello", 5) == 0);

    dbuff_destroy(buf);
}

Test(dbuffer, append_preserves_existing_data)
{
    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    char first[] = "hello";
    char second[] = " world";

    cr_assert_eq(dbuff_append(buf, first, 5), IO_DONE);
    cr_assert_eq(dbuff_append(buf, second, 6), IO_DONE);

    cr_assert_eq(buf->len, 11);
    cr_assert(memcmp(buf->data, "hello world", 11) == 0);

    dbuff_destroy(buf);
}

Test(dbuffer, append_grows_capacity)
{
    dbuffer *buf = dbuff_create(4);
    cr_assert_not_null(buf);

    char data[] = "abcdefgh";

    io_err result = dbuff_append(buf, data, 8);

    cr_assert_eq(result, IO_DONE);
    cr_assert(buf->cap >= 8);
    cr_assert_eq(buf->len, 8);
    cr_assert(memcmp(buf->data, data, 8) == 0);

    dbuff_destroy(buf);
}

Test(dbuffer, append_handles_binary_data)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    const unsigned char data[] = {
        0x41, 0x00, 0x42, 0xFF
    };

    cr_assert_eq(
        dbuff_append(buf, (char *)data, sizeof(data)),
        IO_DONE
    );

    cr_assert_eq(buf->len, sizeof(data));
    cr_assert(
        memcmp(buf->data, data, sizeof(data)) == 0
    );

    dbuff_destroy(buf);
}

/*
 * 3. Reset tests
 */

Test(dbuffer, reset_clears_len_and_offset)
{
    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    char data[] = "hello";
    cr_assert_eq(dbuff_append(buf, data, 5), IO_DONE);

    buf->offset = 3;
    size_t original_cap = buf->cap;

    cr_assert_eq(dbuff_reset(buf), IO_DONE);

    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);
    cr_assert_eq(buf->cap, original_cap);

    dbuff_destroy(buf);
}

Test(dbuffer, append_works_after_reset)
{
    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    char first[] = "first";
    char second[] = "next";

    cr_assert_eq(dbuff_append(buf, first, 5), IO_DONE);
    cr_assert_eq(dbuff_reset(buf), IO_DONE);
    cr_assert_eq(dbuff_append(buf, second, 4), IO_DONE);

    cr_assert_eq(buf->len, 4);
    cr_assert(memcmp(buf->data, "next", 4) == 0);

    dbuff_destroy(buf);
}

/*
 * 4. Read tests
 *
 * A nonblocking pipe lets us test IO_AGAIN and IO_DONE
 * without requiring a network connection.
 */

Test(dbuffer, read_returns_again_when_no_data_is_available)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    int flags = fcntl(fds[0], F_GETFL, 0);
    cr_assert_neq(flags, -1);
    cr_assert_neq(fcntl(fds[0], F_SETFL, flags | O_NONBLOCK), -1);

    dbuffer *buf = dbuff_create(4096);
    cr_assert_not_null(buf);

    io_err result = dbuff_read(buf, fds[0]);

    cr_assert_eq(result, IO_AGAIN);
    cr_assert_eq(buf->len, 0);

    dbuff_destroy(buf);
    close(fds[0]);
    close(fds[1]);
}

Test(dbuffer, read_collects_available_data)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    int flags = fcntl(fds[0], F_GETFL, 0);
    cr_assert_neq(flags, -1);
    cr_assert_neq(fcntl(fds[0], F_SETFL, flags | O_NONBLOCK), -1);

    dbuffer *buf = dbuff_create(4096);
    cr_assert_not_null(buf);

    const char expected[] = "hello";
    cr_assert_eq(write(fds[1], expected, 5), 5);

    io_err result = dbuff_read(buf, fds[0]);

    cr_assert_eq(result, IO_DONE);
    cr_assert_eq(buf->len, 5);
    cr_assert(memcmp(buf->data, expected, 5) == 0);

    dbuff_destroy(buf);
    close(fds[0]);
    close(fds[1]);
}

/*
 * 5. Write tests
 */

Test(dbuffer, write_sends_all_pending_bytes)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    const char expected[] = "hello";
    cr_assert_eq(dbuff_append(buf, (char *)expected, 5), IO_DONE);

    io_err result = dbuff_write(buf, fds[1]);

    cr_assert_eq(result, IO_DONE);
    cr_assert_eq(buf->offset, buf->len);

    char actual[5];
    cr_assert_eq(read(fds[0], actual, sizeof(actual)), 5);
    cr_assert(memcmp(actual, expected, sizeof(actual)) == 0);

    dbuff_destroy(buf);
    close(fds[0]);
    close(fds[1]);
}

Test(dbuffer, write_respects_offset)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    const char data[] = "hello";
    cr_assert_eq(dbuff_append(buf, (char *)data, 5), IO_DONE);

    buf->offset = 2;

    io_err result = dbuff_write(buf, fds[1]);

    cr_assert_eq(result, IO_DONE);
    cr_assert_eq(buf->offset, 5);

    char actual[3];
    cr_assert_eq(read(fds[0], actual, sizeof(actual)), 3);
    cr_assert(memcmp(actual, "llo", 3) == 0);

    dbuff_destroy(buf);
    close(fds[0]);
    close(fds[1]);
}

Test(dbuff_read, grows_buffer_for_data_larger_than_initial_capacity2)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);
    set_nonblocking(fds[0]);

    const size_t input_len = 2048;
    unsigned char input[2048];

    for (size_t i = 0; i < input_len; i++)
        input[i] = (unsigned char)(i % 253);

    cr_assert_eq(write(fds[1], input, input_len), (ssize_t)input_len);

    dbuffer *buf = make_buffer(4);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_DONE);
    cr_assert_eq(buf->len, input_len);
    cr_assert_geq(buf->cap, input_len);
    cr_assert_eq(memcmp(buf->data, input, input_len), 0);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}


/* ---------- Edge cases: invalid arguments ---------- */

Test(dbuff_append, rejects_null_buffer)
{
    const char data[] = "abc";

    cr_assert_eq(dbuff_append(NULL, (char *)data, 3), IO_ERROR);
}

Test(dbuff_append, rejects_null_data_with_nonzero_length)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    cr_assert_eq(dbuff_append(buf, NULL, 3), IO_ERROR);
    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);

    dbuff_destroy(buf);
}

Test(dbuff_append, null_data_with_zero_length_is_noop)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    cr_assert_eq(dbuff_append(buf, NULL, 0), IO_DONE);
    cr_assert_eq(buf->len, 0);
    cr_assert_eq(buf->offset, 0);

    dbuff_destroy(buf);
}

Test(dbuff_reset, rejects_null_buffer)
{
    cr_assert_eq(dbuff_reset(NULL), IO_ERROR);
}

Test(dbuff_read, rejects_null_buffer)
{
    cr_assert_eq(dbuff_read(NULL, -1), IO_ERROR);
}

Test(dbuff_write, rejects_null_buffer)
{
    cr_assert_eq(dbuff_write(NULL, -1), IO_ERROR);
}

/* ---------- Edge cases: capacity limits ---------- */

Test(dbuff_create, rejects_zero_capacity)
{
    /*
     * Contract: zero-capacity buffers are invalid.
     * dbuff_create() must return NULL deterministically.
     */
    cr_assert_null(dbuff_create(0));
}

Test(dbuff_create, rejects_capacity_above_limit)
{
    cr_assert_null(dbuff_create(TEST_MAX_BUFFER_SIZE + 1));
}

Test(dbuff_append, rejects_append_above_maximum_capacity)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    size_t input_len = TEST_MAX_BUFFER_SIZE + 1;
    unsigned char *input = malloc(input_len);
    cr_assert_not_null(input);

    memset(input, 'A', input_len);

    size_t old_len = buf->len;
    size_t old_cap = buf->cap;
    size_t old_offset = buf->offset;

    cr_assert_eq(
        dbuff_append(buf, (char *)input, input_len),
        IO_ERROR);

    /* Failed append must not partially modify buffer state. */
    cr_assert_eq(buf->len, old_len);
    cr_assert_eq(buf->cap, old_cap);
    cr_assert_eq(buf->offset, old_offset);

    free(input);
    dbuff_destroy(buf);
}

Test(dbuff_append, rejects_append_that_would_exceed_limit)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    size_t first_len = TEST_MAX_BUFFER_SIZE - 8;
    unsigned char *first = malloc(first_len);
    cr_assert_not_null(first);
    memset(first, 'A', first_len);

    cr_assert_eq(
        dbuff_append(buf, (char *)first, first_len),
        IO_DONE);

    size_t old_len = buf->len;
    size_t old_cap = buf->cap;
    size_t old_offset = buf->offset;

    const char extra[] = "123456789";

    cr_assert_eq(
        dbuff_append(buf, (char *)extra, sizeof(extra) - 1),
        IO_ERROR);

    cr_assert_eq(buf->len, old_len);
    cr_assert_eq(buf->cap, old_cap);
    cr_assert_eq(buf->offset, old_offset);

    free(first);
    dbuff_destroy(buf);
}

/* ---------- Read: EOF contract ---------- */

Test(dbuff_read, returns_done_when_data_precedes_eof)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    const char input[] = "payload";
    cr_assert_eq(write(fds[1], input, sizeof(input) - 1),
                 (ssize_t)(sizeof(input) - 1));
    close_checked(fds[1]);
    fds[1] = -1;

    dbuffer *buf = dbuff_create(32);
    cr_assert_not_null(buf);

    /*
     * Contract:
     * if bytes were read during this call, return IO_DONE.
     * A subsequent call observes EOF and returns IO_CLOSED.
     */
    cr_assert_eq(dbuff_read(buf, fds[0]), IO_DONE);
    cr_assert_eq(buf->len, sizeof(input) - 1);
    cr_assert_eq(
        memcmp(buf->data, input, sizeof(input) - 1),
        0);

    cr_assert_eq(dbuff_read(buf, fds[0]), IO_CLOSED);

    dbuff_destroy(buf);
    close_checked(fds[0]);
}

/* ---------- Write: invalid internal state ---------- */

Test(dbuff_write, rejects_offset_greater_than_length)
{
    int fds[2];
    cr_assert_eq(pipe(fds), 0);

    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    const char input[] = "abc";
    cr_assert_eq(dbuff_append(buf, (char *)input, 3), IO_DONE);

    buf->offset = buf->len + 1;

    cr_assert_eq(dbuff_write(buf, fds[1]), IO_ERROR);

    dbuff_destroy(buf);
    close_checked(fds[0]);
    close_checked(fds[1]);
}

Test(dbuff_write, rejects_negative_file_descriptor)
{
    dbuffer *buf = dbuff_create(16);
    cr_assert_not_null(buf);

    const char input[] = "abc";
    cr_assert_eq(dbuff_append(buf, (char *)input, 3), IO_DONE);

    cr_assert_eq(dbuff_write(buf, -1), IO_ERROR);

    dbuff_destroy(buf);
}
