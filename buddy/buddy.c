#include "buddy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define TOTAL_MEMORY_PAGES 8192
#define PAGE_SIZE 4096
#define TOTAL_MEMORY_BYTES (TOTAL_MEMORY_PAGES * PAGE_SIZE) // 32MB

#define MIN_ORDER 12 // Smallest block is one page: 2^12 = 4096 bytes
#define MAX_ORDER 25 // Largest block is all memory: 2^25 = 32MB

// A node in a free list.
typedef struct block {
    struct block *next;
} block_t;

// A structure to hold metadata for each page.
typedef struct {
    unsigned char order;   // The order of the block this page belongs to.
    unsigned char is_free; // Flag to indicate if the block is free.
} page_meta_t;

// The start of our simulated memory.
static void *memory_start = NULL;

// Array of free lists, one for each order.
static block_t *free_lists[MAX_ORDER + 1];

// Metadata for all pages.
static page_meta_t page_metadata[TOTAL_MEMORY_PAGES];

// Helper to get the base 2 logarithm, rounding up.
static int ceil_log2(unsigned int n) {
    if (n <= 1)
        return 0;
    int power = 0;
    unsigned int temp = 1;
    while (temp < n) {
        temp <<= 1;
        power++;
    }
    return power;
}

// Get the page index from a memory pointer.
static int ptr_to_page_index(void *ptr) {
    if (ptr == NULL || (char *)ptr < (char *)memory_start) {
        return -1;
    }
    size_t offset = (char *)ptr - (char *)memory_start;
    return offset / PAGE_SIZE;
}

// Get the memory pointer from a page index.
static void *page_index_to_ptr(int index) {
    return (char *)memory_start + (index * PAGE_SIZE);
}

// Get the index of a block's buddy.
static int find_buddy_index(int index, int order) {
    int block_size_in_pages = 1 << (order - MIN_ORDER);
    return index ^ block_size_in_pages;
}

void buddy_init(void) {
    // Use mmap to get a large chunk of memory.
    memory_start = mmap(NULL, TOTAL_MEMORY_BYTES, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_start == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Initialize all free lists to be empty.
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_lists[i] = NULL;
    }

    // The entire memory starts as one big block.
    block_t *initial_block = (block_t *)memory_start;
    initial_block->next = NULL;
    free_lists[MAX_ORDER] = initial_block;

    // Initialize metadata for the first page of the big block.
    page_metadata[0].order = MAX_ORDER;
    page_metadata[0].is_free = 1;

    printf("Buddy system initialized with %d bytes of memory.\n",
           TOTAL_MEMORY_BYTES);
}

// Find the smallest available order that can fit the requested size.
int find_smallest_available_order(int order) {
    static int invalid_order = MAX_ORDER + 1;
    int current_order = invalid_order;

    for (int i = order; i <= MAX_ORDER; i++) {
        if (free_lists[i] != NULL) {
            current_order = i;
            break;
        }
    }
    return current_order;
}

// Take a block from the free list.
struct block *take_block(int current_order) {
    block_t *block = free_lists[current_order];
    free_lists[current_order] = block->next;
    page_metadata[ptr_to_page_index(block)].is_free = 0;

    return block;
}

// Split a block into two smaller blocks.
void split_block(int page_index, int current_order) {
    int buddy_page_index = find_buddy_index(page_index, current_order);
    page_metadata[page_index].order = current_order;

    // Mark the new buddy block and add it to the free list.
    page_metadata[buddy_page_index].order = current_order;
    page_metadata[buddy_page_index].is_free = 1;
    block_t *buddy_block = (block_t *)page_index_to_ptr(buddy_page_index);
    buddy_block->next = free_lists[current_order];
    free_lists[current_order] = buddy_block;
}

void *buddy_alloc(size_t size) {
    if (size == 0)
        return NULL;

    // Calculate the required order for the requested size.
    int order = ceil_log2(size);
    if (order < MIN_ORDER) {
        order = MIN_ORDER;
    }

    if (order > MAX_ORDER) {
        fprintf(stderr, "Error: Requested size %zu is too large.\n", size);
        return NULL;
    }

    // Find the smallest available order that fits.
    int current_order = find_smallest_available_order(order);

    if (current_order > MAX_ORDER) {
        fprintf(stderr, "Error: Not enough memory of size %zu.\n", size);
        return NULL; // No suitable block found.
    }

    // Take the block from the found free list.
    block_t *block = take_block(current_order);
    int page_index = ptr_to_page_index(block);

    // Split the block until it's the correct size.
    while (current_order > order) {
        current_order--;
        split_block(page_index, current_order);
    }

    return (void *)block;
}

void buddy_free(void *ptr) {
    if (ptr == NULL)
        return;

    int page_index = ptr_to_page_index(ptr);
    if (page_index < 0 || page_index >= TOTAL_MEMORY_PAGES) {
        fprintf(stderr, "Error: Cannot free invalid pointer.\n");
        return;
    }

    if (page_metadata[page_index].is_free) {
        fprintf(stderr, "Error: Double free detected for pointer %p.\n", ptr);
        return;
    }

    int current_order = page_metadata[page_index].order;
    page_metadata[page_index].is_free = 1;

    // Coalesce with buddies.
    while (current_order < MAX_ORDER) {
        int buddy_index = find_buddy_index(page_index, current_order);

        // Check if buddy is free and of the same order.
        if (!page_metadata[buddy_index].is_free ||
            page_metadata[buddy_index].order != current_order) {
            break; // Buddy is not available for merging.
        }

        // Buddy is available, so merge.
        // 1. Remove buddy from its free list.
        block_t *prev = NULL;
        block_t *curr = free_lists[current_order];
        while (curr != NULL) {
            if (ptr_to_page_index(curr) == buddy_index) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    free_lists[current_order] = curr->next;
                }
                break;
            }
            prev = curr;
            curr = curr->next;
        }

        // 2. The new, larger block starts at the minimum of the two indices.
        page_index = (page_index < buddy_index) ? page_index : buddy_index;
        current_order++;
        page_metadata[page_index].order = current_order;
    }

    // Add the final (potentially merged) block to the correct free list.
    block_t *new_block = (block_t *)page_index_to_ptr(page_index);
    new_block->next = free_lists[current_order];
    free_lists[current_order] = new_block;
}

void buddy_dump(void) {
    printf("--- Buddy System Free Lists ---\n");
    for (int i = MIN_ORDER; i <= MAX_ORDER; i++) {
        int count = 0;
        block_t *block = free_lists[i];
        while (block != NULL) {
            count++;
            block = block->next;
        }
        if (count > 0) {
            printf("Order %d (size %ld KB): %d blocks\n", i, (1L << i) / 1024, count);
        }
    }
    printf("-----------------------------\n\n");
}
