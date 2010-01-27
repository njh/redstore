
#define EASYCHECK_TESTCASE_START(x)     \
    int main() {                        \
        int n;                          \
        SRunner *sr;                    \
        Suite *s = suite_create(#x);    \
        TCase *tc = tcase_create(#x);
        
        
        
#define EASYCHECK_TESTCASE_ADD(x) tcase_add_test(tc, x);


#define EASYCHECK_TESTCASE_END          \
        suite_add_tcase(s, tc);         \
        sr = srunner_create(s);         \
        srunner_run_all(sr, CK_NORMAL); \
        n = srunner_ntests_failed (sr); \
        srunner_free(sr);               \
        return n;                       \
    }
