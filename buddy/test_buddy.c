#include "buddy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Test framework globals
static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// Test framework macros
#define ASSERT_TRUE(cond, msg) do { \
    total_tests++; \
    if (cond) { \
        passed_tests++; \
        printf(COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", msg); \
    } else { \
        failed_tests++; \
        printf(COLOR_RED "[FAIL]" COLOR_RESET " %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_EQUAL(a, b, msg) ASSERT_TRUE((a) == (b), msg)

#define TEST(name) static void name(void)
#define RUN_TEST(test_func) do { \
    printf("\n" COLOR_YELLOW "Running: %s" COLOR_RESET "\n", #test_func); \
    test_func(); \
} while(0)

// Helper function to count blocks in a specific order
// Note: This requires access to internal structures, so we'll test indirectly

// Test 1: Initialization
TEST(test_init) {
    buddy_init();
    ASSERT_TRUE(true, "buddy_init() executed successfully");
    // After init, we should be able to allocate memory
    void *ptr = buddy_alloc(4096);
    ASSERT_NOT_NULL(ptr, "Can allocate after init");
    buddy_free(ptr);
}

// Test 2: Allocate zero bytes
TEST(test_alloc_zero) {
    void *ptr = buddy_alloc(0);
    ASSERT_NULL(ptr, "Allocating 0 bytes returns NULL");
}

// Test 3: Allocate small memory
TEST(test_alloc_small) {
    void *ptr = buddy_alloc(100);
    ASSERT_NOT_NULL(ptr, "Allocating 100 bytes succeeds");
    buddy_free(ptr);
}

// Test 4: Allocate exact power of two
TEST(test_alloc_exact_power_of_two) {
    void *ptr1 = buddy_alloc(4096);  // 2^12
    ASSERT_NOT_NULL(ptr1, "Allocating 4096 bytes (4KB) succeeds");
    
    void *ptr2 = buddy_alloc(8192);  // 2^13
    ASSERT_NOT_NULL(ptr2, "Allocating 8192 bytes (8KB) succeeds");
    
    buddy_free(ptr1);
    buddy_free(ptr2);
}

// Test 5: Allocate size requiring rounding up
TEST(test_alloc_round_up) {
    void *ptr = buddy_alloc(5000);  // Should round up to 8KB
    ASSERT_NOT_NULL(ptr, "Allocating 5000 bytes succeeds");
    buddy_free(ptr);
}

// Test 6: Allocate very large memory (should fail)
TEST(test_alloc_too_large) {
    void *ptr = buddy_alloc(64UL * 1024 * 1024);  // 64MB > 32MB total
    ASSERT_NULL(ptr, "Allocating 64MB (exceeds total) returns NULL");
}

// Test 7: Multiple allocations (split testing)
TEST(test_multiple_alloc_split) {
    void *ptr1 = buddy_alloc(4096);   // 4KB
    void *ptr2 = buddy_alloc(4096);   // 4KB
    void *ptr3 = buddy_alloc(8192);   // 8KB
    
    ASSERT_NOT_NULL(ptr1, "First allocation succeeds");
    ASSERT_NOT_NULL(ptr2, "Second allocation succeeds");
    ASSERT_NOT_NULL(ptr3, "Third allocation succeeds");
    
    // All pointers should be different
    ASSERT_TRUE(ptr1 != ptr2, "ptr1 and ptr2 are different");
    ASSERT_TRUE(ptr1 != ptr3, "ptr1 and ptr3 are different");
    ASSERT_TRUE(ptr2 != ptr3, "ptr2 and ptr3 are different");
    
    buddy_free(ptr1);
    buddy_free(ptr2);
    buddy_free(ptr3);
}

// Test 8: Free NULL pointer
TEST(test_free_null) {
    buddy_free(NULL);
    ASSERT_TRUE(true, "Freeing NULL pointer doesn't crash");
}

// Test 9: Free and reallocate
TEST(test_free_and_realloc) {
    void *ptr1 = buddy_alloc(4096);
    ASSERT_NOT_NULL(ptr1, "Initial allocation succeeds");
    
    buddy_free(ptr1);
    
    void *ptr2 = buddy_alloc(4096);
    ASSERT_NOT_NULL(ptr2, "Reallocation after free succeeds");
    
    // After freeing and reallocating same size, might get same pointer
    // (but not guaranteed due to coalescing)
    
    buddy_free(ptr2);
}

// Test 10: Simple coalescing
TEST(test_coalesce_simple) {
    // Allocate two buddy blocks
    void *ptr1 = buddy_alloc(64 * 1024);  // 64KB
    void *ptr2 = buddy_alloc(64 * 1024);  // 64KB
    
    ASSERT_NOT_NULL(ptr1, "First 64KB allocation succeeds");
    ASSERT_NOT_NULL(ptr2, "Second 64KB allocation succeeds");
    
    // Free both - they should coalesce
    buddy_free(ptr1);
    buddy_free(ptr2);
    
    // Now we should be able to allocate a larger block
    void *ptr3 = buddy_alloc(128 * 1024);  // 128KB
    ASSERT_NOT_NULL(ptr3, "Can allocate 128KB after coalescing");
    
    buddy_free(ptr3);
}

// Test 11: Cascade coalescing
TEST(test_coalesce_cascade) {
    // Allocate multiple blocks
    void *p1 = buddy_alloc(70 * 1024);   // 128KB
    void *p2 = buddy_alloc(35 * 1024);   // 64KB
    void *p3 = buddy_alloc(120 * 1024);  // 128KB
    void *p4 = buddy_alloc(8 * 1024);    // 8KB
    
    ASSERT_NOT_NULL(p1, "p1 allocation succeeds");
    ASSERT_NOT_NULL(p2, "p2 allocation succeeds");
    ASSERT_NOT_NULL(p3, "p3 allocation succeeds");
    ASSERT_NOT_NULL(p4, "p4 allocation succeeds");
    
    // Free in specific order to trigger cascade
    buddy_free(p2);
    buddy_free(p4);
    buddy_free(p1);
    buddy_free(p3);
    
    // After all frees, system should return to initial state
    // We should be able to allocate large block
    void *large = buddy_alloc(8 * 1024 * 1024);  // 8MB
    ASSERT_NOT_NULL(large, "Can allocate 8MB after cascade coalescing");
    
    buddy_free(large);
}

// Test 12: Double free detection (will output error message)
TEST(test_double_free) {
    void *ptr = buddy_alloc(4096);
    ASSERT_NOT_NULL(ptr, "Allocation succeeds");
    
    buddy_free(ptr);
    
    // This should trigger error message but not crash
    printf("  (Expecting error message for double free...)\n");
    buddy_free(ptr);
    
    ASSERT_TRUE(true, "Double free doesn't crash (error message expected)");
}

// Test 13: Allocate all memory
TEST(test_alloc_all_memory) {
    // Allocate the entire 32MB
    void *ptr = buddy_alloc(32 * 1024 * 1024);
    ASSERT_NOT_NULL(ptr, "Can allocate all 32MB");
    
    // Try to allocate more - should fail
    void *ptr2 = buddy_alloc(4096);
    ASSERT_NULL(ptr2, "Cannot allocate when memory is full");
    
    buddy_free(ptr);
    
    // Now should be able to allocate again
    void *ptr3 = buddy_alloc(4096);
    ASSERT_NOT_NULL(ptr3, "Can allocate after freeing all memory");
    buddy_free(ptr3);
}

// Test 14: Minimum allocation
TEST(test_alloc_minimum) {
    void *ptr = buddy_alloc(1);  // 1 byte, should round up to 4KB
    ASSERT_NOT_NULL(ptr, "Allocating 1 byte succeeds");
    buddy_free(ptr);
}

// Test 15: Write and read allocated memory
TEST(test_memory_access) {
    size_t size = 1024;
    char *ptr = (char *)buddy_alloc(size);
    ASSERT_NOT_NULL(ptr, "Allocation succeeds");
    
    // Write to memory
    for (size_t i = 0; i < size; i++) {
        ptr[i] = (char)(i % 256);
    }
    
    // Read back and verify
    bool correct = true;
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != (char)(i % 256)) {
            correct = false;
            break;
        }
    }
    
    ASSERT_TRUE(correct, "Can write and read allocated memory correctly");
    buddy_free(ptr);
}

// Test 16: Fragmentation scenario
TEST(test_fragmentation) {
    // Allocate alternating sizes to create fragmentation
    void *ptrs[10];
    
    for (int i = 0; i < 10; i++) {
        size_t size = (i % 2 == 0) ? 4096 : 8192;
        ptrs[i] = buddy_alloc(size);
        ASSERT_NOT_NULL(ptrs[i], "Fragmentation allocation succeeds");
    }
    
    // Free every other block
    for (int i = 0; i < 10; i += 2) {
        buddy_free(ptrs[i]);
    }
    
    // Free remaining blocks
    for (int i = 1; i < 10; i += 2) {
        buddy_free(ptrs[i]);
    }
    
    ASSERT_TRUE(true, "Fragmentation scenario handled");
}

// Test 17: Stress test - many allocations
TEST(test_stress_many_allocs) {
    const int num_allocs = 100;
    void *ptrs[num_allocs];
    int successful = 0;
    
    // Allocate many small blocks
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = buddy_alloc(4096);
        if (ptrs[i] != NULL) {
            successful++;
        }
    }
    
    ASSERT_TRUE(successful > 0, "At least some allocations succeed in stress test");
    
    // Free all successful allocations
    for (int i = 0; i < num_allocs; i++) {
        if (ptrs[i] != NULL) {
            buddy_free(ptrs[i]);
        }
    }
}

// Test 18: Allocation pattern from main.c demo
TEST(test_demo_pattern) {
    void *p1 = buddy_alloc(70 * 1024);
    void *p2 = buddy_alloc(35 * 1024);
    void *p3 = buddy_alloc(120 * 1024);
    void *p4 = buddy_alloc(8 * 1024);
    
    ASSERT_NOT_NULL(p1, "Demo p1 allocation succeeds");
    ASSERT_NOT_NULL(p2, "Demo p2 allocation succeeds");
    ASSERT_NOT_NULL(p3, "Demo p3 allocation succeeds");
    ASSERT_NOT_NULL(p4, "Demo p4 allocation succeeds");
    
    buddy_free(p2);
    buddy_free(p4);
    buddy_free(p1);
    buddy_free(p3);
    
    ASSERT_TRUE(true, "Demo pattern completes successfully");
}

// Test 19: Large block allocation
TEST(test_large_block) {
    void *ptr = buddy_alloc(8 * 1024 * 1024);  // 8MB
    ASSERT_NOT_NULL(ptr, "Can allocate 8MB block");
    buddy_free(ptr);
}

// Test 20: Sequential alloc/free cycles
TEST(test_sequential_cycles) {
    for (int cycle = 0; cycle < 5; cycle++) {
        void *ptr = buddy_alloc(16 * 1024);
        ASSERT_NOT_NULL(ptr, "Sequential cycle allocation succeeds");
        buddy_free(ptr);
    }
    ASSERT_TRUE(true, "Sequential alloc/free cycles complete");
}

// Print test summary
void print_test_summary(void) {
    printf("\n");
    printf("========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total tests:  %d\n", total_tests);
    printf(COLOR_GREEN "Passed:       %d" COLOR_RESET "\n", passed_tests);
    if (failed_tests > 0) {
        printf(COLOR_RED "Failed:       %d" COLOR_RESET "\n", failed_tests);
    } else {
        printf("Failed:       %d\n", failed_tests);
    }
    printf("========================================\n");
    
    if (failed_tests == 0) {
        printf(COLOR_GREEN "All tests passed! ✓" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Some tests failed! ✗" COLOR_RESET "\n");
    }
}

int main(void) {
    printf("========================================\n");
    printf("Buddy System Unit Tests\n");
    printf("========================================\n");
    
    // Run all tests
    RUN_TEST(test_init);
    RUN_TEST(test_alloc_zero);
    RUN_TEST(test_alloc_small);
    RUN_TEST(test_alloc_exact_power_of_two);
    RUN_TEST(test_alloc_round_up);
    RUN_TEST(test_alloc_too_large);
    RUN_TEST(test_multiple_alloc_split);
    RUN_TEST(test_free_null);
    RUN_TEST(test_free_and_realloc);
    RUN_TEST(test_coalesce_simple);
    RUN_TEST(test_coalesce_cascade);
    RUN_TEST(test_double_free);
    RUN_TEST(test_alloc_all_memory);
    RUN_TEST(test_alloc_minimum);
    RUN_TEST(test_memory_access);
    RUN_TEST(test_fragmentation);
    RUN_TEST(test_stress_many_allocs);
    RUN_TEST(test_demo_pattern);
    RUN_TEST(test_large_block);
    RUN_TEST(test_sequential_cycles);
    
    print_test_summary();
    
    return (failed_tests == 0) ? 0 : 1;
}
