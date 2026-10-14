#include <criterion/criterion.h>
#include <stddef.h>
#include <string.h>

#include "../../src/http/router.h"
#include "../../src/parser/http_parser.h"

/*
 * Build a minimal request without parsing raw HTTP data.
 * find_router() only needs the method and path for these tests.
 */
static http_request make_request(http_method method, const char *path)
{
    http_request req = {0};

    req.mtd = method;
    req.path.ptr = (char *)path;
    req.path.len = strlen(path);

    return req;
}

/* ---------- Exact routes ---------- */

Test(router, get_root_matches_exact_route)
{
    http_request req = make_request(HTTP_GET, "/");

    router *r = find_router(&req);

    cr_assert_not_null(r);
    cr_assert_eq(r->type, EXACT_ROUTE);
    cr_assert_str_eq(r->path, "/");
    cr_assert_eq(r->rout_handler, root_handler);
}

Test(router, root_does_not_match_other_paths)
{
    http_request req = make_request(HTTP_GET, "/abc");

    cr_assert_null(find_router(&req));
}

Test(router, root_does_not_match_trailing_slash)
{
    http_request req = make_request(HTTP_GET, "//");

    cr_assert_null(find_router(&req));
}

/* ---------- Prefix routes ---------- */

Test(router, static_root_matches_prefix_route)
{
    http_request req = make_request(HTTP_GET, "/static");

    router *r = find_router(&req);

    cr_assert_not_null(r);
    cr_assert_eq(r->type, PREFIX_ROUTE);
    cr_assert_str_eq(r->path, "/static");
    cr_assert_eq(r->rout_handler, static_handler);
}

Test(router, static_file_matches_prefix_route)
{
    http_request req =
        make_request(HTTP_GET, "/static/index.html");

    router *r = find_router(&req);

    cr_assert_not_null(r);
    cr_assert_eq(r->rout_handler, static_handler);
}

Test(router, nested_static_path_matches_prefix_route)
{
    http_request req =
        make_request(HTTP_GET, "/static/css/main.css");

    router *r = find_router(&req);

    cr_assert_not_null(r);
    cr_assert_eq(r->rout_handler, static_handler);
}

Test(router, shorter_prefix_does_not_match)
{
    http_request req = make_request(HTTP_GET, "/stat");

    cr_assert_null(find_router(&req));
}

/*
 * Desired behavior:
 * /staticXYZ is not a child of /static.
 *
 * This test is expected to FAIL with the current implementation.
 */
Test(router, prefix_requires_path_boundary)
{
    http_request req = make_request(HTTP_GET, "/staticXYZ");

    cr_assert_null(find_router(&req));
}

/* ---------- HTTP methods ---------- */

Test(router, post_root_does_not_match_get_route)
{
    http_request req = make_request(HTTP_POST, "/");

    cr_assert_null(find_router(&req));
}

Test(router, post_static_does_not_match_get_route)
{
    http_request req =
        make_request(HTTP_POST, "/static/index.html");

    cr_assert_null(find_router(&req));
}

/* ---------- Empty and unusual paths ---------- */

Test(router, empty_path_does_not_match)
{
    http_request req = make_request(HTTP_GET, "");

    cr_assert_null(find_router(&req));
}

Test(router, unrelated_path_does_not_match)
{
    http_request req = make_request(HTTP_GET, "/api/test");

    cr_assert_null(find_router(&req));
}

/* ---------- Lookup independence ---------- */

Test(router, lookup_results_are_independent)
{
    http_request root = make_request(HTTP_GET, "/");
    http_request missing = make_request(HTTP_GET, "/missing");
    http_request file =
        make_request(HTTP_GET, "/static/index.html");

    router *root_route = find_router(&root);

    cr_assert_not_null(root_route);
    cr_assert_eq(root_route->rout_handler, root_handler);

    cr_assert_null(find_router(&missing));

    router *static_route = find_router(&file);

    cr_assert_not_null(static_route);
    cr_assert_eq(static_route->rout_handler, static_handler);

    /* A previous lookup must not change the root lookup result. */
    cr_assert_eq(find_router(&root), root_route);
}
