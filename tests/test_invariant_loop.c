#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Include the actual production header
#include "src/loop.h"

START_TEST(test_su_to_str_buffer_safety)
{
    // Invariant: su_to_str must never write beyond the bounds of its destination buffer
    union sockaddr_union test_addrs[3];
    char buffer[256];
    
    // Initialize test cases
    // 1. Valid input (normal case)
    memset(&test_addrs[0], 0, sizeof(union sockaddr_union));
    test_addrs[0].v4.sin_family = AF_INET;
    test_addrs[0].v4.sin_port = htons(80);
    
    // 2. Boundary case - maximum port number
    memset(&test_addrs[1], 0, sizeof(union sockaddr_union));
    test_addrs[1].v4.sin_family = AF_INET;
    test_addrs[1].v4.sin_port = htons(65535);
    
    // 3. Adversarial case - IPv6 with port (tests both code paths)
    memset(&test_addrs[2], 0, sizeof(union sockaddr_union));
    test_addrs[2].v6.sin6_family = AF_INET6;
    test_addrs[2].v6.sin6_port = htons(8080);
    
    for (int i = 0; i < 3; i++) {
        // Clear buffer and add sentinel values
        memset(buffer, 'A', sizeof(buffer));
        buffer[255] = '\0';  // Ensure null termination
        
        // Call the actual production function
        su_to_str(&test_addrs[i], buffer);
        
        // Security property: buffer[255] must remain '\0' (no overflow)
        ck_assert_msg(buffer[255] == '\0', 
                     "Buffer overflow detected for test case %d", i);
        
        // Additional check: ensure string is properly null-terminated
        size_t len = strlen(buffer);
        ck_assert_msg(len < 256, 
                     "String length %zu exceeds buffer size for test case %d", 
                     len, i);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_su_to_str_buffer_safety);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}