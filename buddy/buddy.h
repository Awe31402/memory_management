#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>

// Initializes the buddy system. Must be called before any other buddy function.
void buddy_init(void);

// Allocates a block of memory of at least 'size' bytes.
void *buddy_alloc(size_t size);

// Frees a previously allocated block of memory.
void buddy_free(void *ptr);

// Prints the current state of the free lists for debugging purposes.
void buddy_dump(void);

#endif // BUDDY_H
