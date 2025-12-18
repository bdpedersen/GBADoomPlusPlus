#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <csignal>

// Test harness that runs both gmalloc test programs and reports results
int main() {
    printf("=== gmalloc Test Harness ===\n\n");
    
    int total_tests = 4;
    int tests_passed = 0;
    
    // Test 1: Correct usage should succeed (exit code 0, no crash)
    printf("Test 1: gmalloc_correctuse (should succeed)\n");
    printf("  Running gmalloc_correctuse...\n");
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Child process: run the correct usage test
        execl("./build/gmalloc_correctuse", "gmalloc_correctuse", NULL);
        perror("Failed to exec gmalloc_correctuse");
        exit(1);
    } else if (pid1 > 0) {
        // Parent process: wait for child
        int status;
        waitpid(pid1, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                printf("  ✓ PASS: gmalloc_correctuse exited successfully\n\n");
                tests_passed++;
            } else {
                printf("  ✗ FAIL: gmalloc_correctuse exited with code %d\n\n", exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            printf("  ✗ FAIL: gmalloc_correctuse was killed by signal %d\n\n", signal);
        }
    } else {
        perror("fork failed");
        return 1;
    }
    
    // Test 2: Use-after-free should crash (killed by SIGSEGV)
    printf("Test 2: gmalloc_useafterfree (should crash with SIGSEGV)\n");
    printf("  Running gmalloc_useafterfree...\n");
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Child process: run the use-after-free test
        execl("./build/gmalloc_useafterfree", "gmalloc_useafterfree", NULL);
        perror("Failed to exec gmalloc_useafterfree");
        exit(1);
    } else if (pid2 > 0) {
        // Parent process: wait for child
        int status;
        waitpid(pid2, &status, 0);
        
        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            // Accept both SIGSEGV (11) and SIGBUS (10) since different systems handle
            // page protection violations differently
            if (signal == SIGSEGV || signal == SIGBUS) {
                printf("  ✓ PASS: gmalloc_useafterfree crashed with signal %d as expected\n\n", signal);
                tests_passed++;
            } else {
                printf("  ✗ FAIL: gmalloc_useafterfree was killed by signal %d (expected SIGSEGV or SIGBUS)\n\n", signal);
            }
        } else if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("  ✗ FAIL: gmalloc_useafterfree exited with code %d (expected crash)\n\n", exit_code);
        }
    } else {
        perror("fork failed");
        return 1;
    }
    
    // Test 3: Buffer overflow - currently this doesn't trigger the guard page
    // The implementation may have issues with upper guard page detection
    printf("Test 3: gmalloc_bufferoverflow (upper guard page detection)\n");
    printf("  Running gmalloc_bufferoverflow...\n");
    
    pid_t pid3 = fork();
    if (pid3 == 0) {
        // Child process: run the buffer overflow test
        execl("./build/gmalloc_bufferoverflow", "gmalloc_bufferoverflow", NULL);
        perror("Failed to exec gmalloc_bufferoverflow");
        exit(1);
    } else if (pid3 > 0) {
        // Parent process: wait for child
        int status;
        waitpid(pid3, &status, 0);
        
        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            // Accept both SIGSEGV (11) and SIGBUS (10) since different systems handle
            // page protection violations differently
            if (signal == SIGSEGV || signal == SIGBUS) {
                printf("  ✓ PASS: gmalloc_bufferoverflow crashed with signal %d as expected\n\n", signal);
                tests_passed++;
            } else {
                printf("  ✗ FAIL: gmalloc_bufferoverflow was killed by signal %d (expected SIGSEGV or SIGBUS)\n\n", signal);
            }
        } else if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("  ✗ FAIL: gmalloc_bufferoverflow exited with code %d (expected crash)\n\n", exit_code);
        }
    } else {
        perror("fork failed");
        return 1;
    }
    
    // Test 4: Buffer underflow should crash with SIGSEGV/SIGBUS
    printf("Test 4: gmalloc_bufferunderflow (should crash with SIGSEGV/SIGBUS)\n");
    printf("  Running gmalloc_bufferunderflow...\n");
    
    pid_t pid4 = fork();
    if (pid4 == 0) {
        // Child process: run the buffer underflow test
        execl("./build/gmalloc_bufferunderflow", "gmalloc_bufferunderflow", NULL);
        perror("Failed to exec gmalloc_bufferunderflow");
        exit(1);
    } else if (pid4 > 0) {
        // Parent process: wait for child
        int status;
        waitpid(pid4, &status, 0);
        
        if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            // Accept both SIGSEGV (11) and SIGBUS (10) since different systems handle
            // page protection violations differently
            if (signal == SIGSEGV || signal == SIGBUS) {
                printf("  ✓ PASS: gmalloc_bufferunderflow crashed with signal %d as expected\n\n", signal);
                tests_passed++;
            } else {
                printf("  ✗ FAIL: gmalloc_bufferunderflow was killed by signal %d (expected SIGSEGV or SIGBUS)\n\n", signal);
            }
        } else if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("  ✗ FAIL: gmalloc_bufferunderflow exited with code %d (expected crash)\n\n", exit_code);
        }
    } else {
        perror("fork failed");
        return 1;
    }
    
    // Summary
    printf("=== Test Results ===\n");
    printf("Passed: %d/%d tests\n", tests_passed, total_tests);
    
    if (tests_passed == total_tests) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}
