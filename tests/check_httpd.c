#include <check.h>
#include <unistd.h>
#include <stdlib.h>

#include "http/redhttpd.h"


START_TEST (test_http_url_unescape)
{
    const char* escaped = "hello+world";
    char * unescaped = http_url_unescape(escaped);
    fail_unless(strcmp(unescaped, "hello world")==0, NULL);
    free(unescaped);
}
END_TEST


Suite* redhttpd_suite(void)
{
    Suite *s = suite_create ("redhttpd");
    
    /* URI test case */
    TCase *tc_uri = tcase_create ("URI");
    tcase_add_test(tc_uri, test_http_url_unescape);
    suite_add_tcase(s, tc_uri);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = redhttpd_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
