#include "buddy.h"
#include <stdio.h>

int main() {
    // Initialize the buddy system.
    buddy_init();
    buddy_dump();

    // --- Allocation Phase ---
    printf("Allocating block p1 of size 70KB...\n");
    void *p1 = buddy_alloc(70 * 1024); // Needs 128KB block (order 17)
    buddy_dump();

    printf("Allocating block p2 of size 35KB...\n");
    void *p2 = buddy_alloc(35 * 1024); // Needs 64KB block (order 16)
    buddy_dump();

    printf("Allocating block p3 of size 120KB...\n");
    void *p3 = buddy_alloc(120 * 1024); // Needs 128KB block (order 17)
    buddy_dump();

    printf("Allocating block p4 of size 8KB...\n");
    void *p4 = buddy_alloc(8 * 1024); // Needs 8KB block (order 13)
    buddy_dump();

    // --- Deallocation and Coalescing Phase ---
    printf("Freeing p2 (35KB). Should merge with its buddy.\n");
    buddy_free(p2);
    buddy_dump();

    printf("Freeing p4 (8KB). Should not merge yet.\n");
    buddy_free(p4);
    buddy_dump();

    printf("Freeing p1 (70KB). Should cause a cascade of merges.\n");
    buddy_free(p1);
    buddy_dump();

    printf("Freeing p3 (120KB). System should return to initial state.\n");
    buddy_free(p3);
    buddy_dump();

    // --- Edge Case: Allocate a very large block ---
    printf("Attempting to allocate a large block of 8MB...\n");
    void *large_block =
        buddy_alloc(8 * 1024 * 1024); // Needs 8MB block (order 23)
    buddy_dump();

    printf("Freeing the large block...\n");
    buddy_free(large_block);
    buddy_dump();

    printf("Demo finished.\n");

    return 0;
}
