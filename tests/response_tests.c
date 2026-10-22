#include "minunit.h"

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sds.h>

#include <config.h>
#include <mime.h>
#include <response.h>

static const char *
header_value(Response *response, const char *key)
{
    for (int i = 0; i < response->headers_count; i++) {
        if (strcmp(response->headers[i]->key, key) == 0) {
            return response->headers[i]->value;
        }
    }
    return NULL;
}

char *
test_content_length_matches_body_for_a_real_file()
{
    char dir[] = "/tmp/bode_ok_XXXXXX";
    mu_assert(mkdtemp(dir) != NULL, "mkdtemp failed");

    char path[256];
    snprintf(path, sizeof path, "%s/hello.txt", dir);
    FILE *f = fopen(path, "w");
    fputs("hello\n", f);          // 6 bytes
    fclose(f);

    Config *config = config_create();
    MimeTypes *types = mime_types_load(config);

    Response *response = response_create(path, types);

    mu_assert(response->status->code == 200, "a readable file should be 200");
    const char *cl = header_value(response, "Content-Length");
    mu_assert(cl != NULL && atoi(cl) == 6,
              "Content-Length should equal the file size");

    response_free(response);
    mime_types_free(types);
    config_free(config);
    unlink(path);
    rmdir(dir);
    return NULL;
}

char *
test_read_error_is_500_with_real_content_length()
{
    /* Force a mid-read failure through the public interface: a directory named
       "index.html". file_path_for() appends index.html to the requested dir;
       fopen() opens the directory fine, but fread() then sets ferror. */
    char dir[] = "/tmp/bode_re_XXXXXX";
    mu_assert(mkdtemp(dir) != NULL, "mkdtemp failed");

    char index[256];
    snprintf(index, sizeof index, "%s/index.html", dir);
    mu_assert(mkdir(index, 0755) == 0, "mkdir index.html dir failed");

    Config *config = config_create();
    MimeTypes *types = mime_types_load(config);

    Response *response = response_create(dir, types);

    mu_assert(response->status->code == 500,
              "a mid-read failure should produce a 500, not a 200");

    const char *cl = header_value(response, "Content-Length");
    mu_assert(cl != NULL && strcmp(cl, "-1") != 0 && atoi(cl) >= 0,
              "Content-Length must be a real length, never -1");
    mu_assert((size_t) atoi(cl) == sdslen(response->bode),
              "Content-Length must match the actual body length");

    response_free(response);
    mime_types_free(types);
    config_free(config);
    rmdir(index);
    rmdir(dir);
    return NULL;
}

char *
all_tests()
{
    mu_suite_start();
    mu_run_test(test_content_length_matches_body_for_a_real_file);
    mu_run_test(test_read_error_is_500_with_real_content_length);
    return NULL;
}

RUN_TESTS(all_tests);
