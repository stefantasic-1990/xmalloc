#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define MEMBLOCK_HEADER_SIZE sizeof(struct memBlock) // define the size of the memory block header
#define WORD_SIZE sizeof(void*) // define the word size of the system to be the size of a pointer in bytes

// memory block struct
struct memBlock {
    size_t size;          // size of the memory block in bytes
    struct memBlock* next;   // pointer to the next memory block in the free list
};

static struct memBlock* free_list = NULL; // global pointer to the free list (the first element in the free list)

// memory allocation function
void* xmalloc(size_t size) {
    // if requested size is zero, return a NULL
    if (size == 0) return NULL;

    // adjust size to include header and align it
    int block_size = MEMBLOCK_HEADER_SIZE + size;
    int aligned_block_size = (block_size + WORD_SIZE - 1) & ~(WORD_SIZE - 1); // power of 2 alignment

    // traverse the free list to find a suitable block
    struct memBlock* prev = NULL;
    struct memBlock* curr = free_list;

    while (curr) { // while the current block is not NULL
        if (curr->size >= size) {
            // remove block from the free list and return a pointer to the memory
            if (curr->next == NULL) {
                // only node
                if (prev == NULL) {
                    free_list = NULL;
                // last node
                } else {
                    prev->next = NULL;
                    }
            } else {
                // first node
                if (prev == NULL) {
                    free_list = curr->next;
                    curr->next = NULL;
                // middle node
                } else {
                    prev->next = curr->next;
                    curr->next = NULL;
                }
            }
            return (void*)((char*)curr + MEMBLOCK_HEADER_SIZE);
        }
        prev = curr;
        curr = curr->next;
    }

    // if no suitable block is found, request more memory from the system using sbrk
    curr = (struct memBlock*)sbrk(aligned_block_size); // increase the program break (heap) by the amount needed and cast the starting adress to memBlock type
    if (curr == (void*)-1) return NULL; // if sbrk fails, return NULL

    // initialize the new block and return a pointer to it
    curr->size = aligned_block_size - MEMBLOCK_HEADER_SIZE;
    curr->next = NULL;
    return (void*)((char*)curr + MEMBLOCK_HEADER_SIZE);
}

// memory deallocation function
void xfree(void* ptr) {
    if (!ptr) return;

    // get the block header from the pointer
    struct memBlock* block = (struct memBlock*)((char*)ptr - MEMBLOCK_HEADER_SIZE);

    // add the block to the free list
    block->next = free_list;
    free_list = block;
}

void xprintfl() {
    struct memBlock* curr = free_list;
    int i = 1;

    while (curr) {
        printf("Free list block %i has a data size of %zu bytes\n", i, curr->size);
        curr = curr->next;
        i++;
    }
}