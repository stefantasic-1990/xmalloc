#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

#define ARENA_SIZE (16 * 1024 * 1024) // 16 MB
#define WORD_SIZE sizeof(void*)
// memory aligned header and footer sizes
#define MEMBLOCK_HEADER_SIZE ((sizeof(struct blockHeader) + WORD_SIZE - 1) & ~(WORD_SIZE - 1))
#define MEMBLOCK_FOOTER_SIZE ((sizeof(struct blockFooter) + WORD_SIZE - 1) & ~(WORD_SIZE - 1))
// smallest possible block size (holds one word)
#define SMALLEST_MEMBLOCK_SIZE (MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + WORD_SIZE)

// memory block header
struct blockHeader {
    struct blockHeader* next;
    struct blockHeader* prev;
    size_t size;
    bool free;
};

// memory block footer
struct blockFooter {
    size_t size;
    bool free;
};

static struct blockHeader* freelist = NULL;

// print the current free list details
void yprintfl() {
    struct blockHeader* block = freelist;
    int i = 1; // block number

    while (block) {
        printf("Free list block (%i) has a data size of (%zu) bytes. Header: %i Footer: %i \n", i, block->size, block->free, block->free);
        block = block->next;
        i++;
    }
    printf("\n");
}

// map additional memory into free list
void mapMoreMemory() {
    // map a large block of memory
    void* arena = mmap(
        NULL, 
        ARENA_SIZE, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS, 
        -1, 
        0
    );

    if (arena == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // initialize header and footer, add to free list
    struct blockHeader* arena_header = arena;
    arena_header->next = freelist;
    arena_header->prev = NULL;
    arena_header->size = (ARENA_SIZE - MEMBLOCK_HEADER_SIZE - MEMBLOCK_FOOTER_SIZE);
    arena_header->free = true;

    struct blockFooter* arena_footer = (void*)((char*)arena_header + MEMBLOCK_HEADER_SIZE + arena_header->size);
    arena_footer->size = arena_header->size;
    arena_footer->free = arena_header->free;

    if (freelist != NULL) freelist->prev = arena_header;
    freelist = arena_header;

    return;
}

// remove memory block from free list
void removeBlock(struct blockHeader* block) {
    if (block->next == NULL) {
        // only node
        if (block->prev == NULL) {
            freelist = NULL;
        // last node
        } else {
            block->prev->next = NULL;
            block->prev = NULL;
        }
    } else {
        // first node
        if (block->prev == NULL) {
            freelist = block->next;
            block->next->prev = NULL;
            block->next = NULL;
        // middle node
        } else {
            block->next->prev = block->prev;
            block->prev->next = block->next;
            block->next = NULL;
            block->prev = NULL;
        }
    }

    // change block free status in header and footer
    struct blockFooter* block_footer = (void*)((char*)block + MEMBLOCK_HEADER_SIZE + block->size);
    block_footer->free = false;
    block->free = block_footer->free;

    return;
}

// check resulting size if this block is coalesced
int checkCoalesceSize(struct blockHeader* block) {
    int coalesced_size = block->size;// = block->size;
    if (block->next != NULL) coalesced_size += block->next->size + MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE;
    if (block->prev != NULL) coalesced_size += block->prev->size + MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE;

    printf("Size before coalescance: %i\n", coalesced_size);

    return coalesced_size;
}

// coalesce memory block
void* blockCoalesce(struct blockHeader* block) {
    struct blockHeader* new_block;

    printf("Coalescing block with size %zu\n", block->size);

    // check if block on the right is free to coalesce
    struct blockHeader* next_block_header = (void*)((char*)block + MEMBLOCK_HEADER_SIZE + block->size + MEMBLOCK_FOOTER_SIZE);
    if (next_block_header->free == true) {
        block->size += MEMBLOCK_FOOTER_SIZE + MEMBLOCK_HEADER_SIZE + next_block_header->size;
        struct blockFooter* new_block_footer = (void*)((char*)block + MEMBLOCK_HEADER_SIZE + block->size);
        new_block_footer->size = block->size;
        new_block = block;
        removeBlock(next_block_header);
        new_block_footer->free = true;
        printf("Next coalesced.\n");
    }

    // check if block on the left is free to coalesce
    struct blockFooter* prev_block_footer = (void*)((char*)block - MEMBLOCK_FOOTER_SIZE);
    if (prev_block_footer->free == true) {
        struct blockHeader* new_block_header = (void*)((char*)prev_block_footer - prev_block_footer->size - MEMBLOCK_HEADER_SIZE);
        new_block_header->size += MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + block->size;
        struct blockFooter* new_block_footer = (void*)((char*)new_block_header + MEMBLOCK_HEADER_SIZE + new_block_header->size);
        new_block_footer->size = new_block_header->size;
        new_block = new_block_header;
        removeBlock(block);
        new_block_footer->free = true;
        printf("Previous coalesced.\n");
    }

    return new_block;
}

// split memory block
void* blockSplit(struct blockHeader* block, size_t size) {
    // align size to memory
    size_t aligned_size = (size + WORD_SIZE - 1) & ~(WORD_SIZE - 1);

    printf("Block size pre split is: %zu\n", block->size);

    // get new block footer
    struct blockFooter* block_footer = (void*)((char*)block + MEMBLOCK_HEADER_SIZE + aligned_size);
    // get split block header
    struct blockHeader* split_block_header = (void*)((char*)block_footer + MEMBLOCK_FOOTER_SIZE);
    // get split block footer
    struct blockFooter* split_block_footer = (void*)((char*)split_block_header + MEMBLOCK_HEADER_SIZE + (block->size - aligned_size - MEMBLOCK_HEADER_SIZE - MEMBLOCK_FOOTER_SIZE));

    // adjust next and prev pointers
    split_block_header->next = block->next;
    if (split_block_header->next != NULL) split_block_header->next->prev = split_block_header;;
    split_block_header->prev = block;
    block->next = split_block_header;

    // adjust size and free flag
    split_block_header->size = block->size - aligned_size - MEMBLOCK_HEADER_SIZE - MEMBLOCK_FOOTER_SIZE;
    split_block_header->free = true;
    split_block_footer->size = split_block_header->size;
    split_block_footer->free = split_block_header->free;
    block->size = aligned_size;
    block->free = true;
    block_footer->size = block->size;
    block_footer->free = true;

    return block;
}

// allocate memory from the free list
void* ymalloc(size_t size) {
    // if requested block size is zero, return NULL
    if (size == 0) return NULL;

    // adjust requested size to account for memory alignment and header/footer
    size_t aligned_size = (MEMBLOCK_HEADER_SIZE + MEMBLOCK_FOOTER_SIZE + size + WORD_SIZE - 1) & ~(WORD_SIZE -1);

    struct blockHeader* block = freelist;

    // search through the free list for a suitable block
    while (block) {
        // if block size is larger than the request size and can be split
        if (block->size >= (size + SMALLEST_MEMBLOCK_SIZE)) {
            struct blockHeader* allocated_block = blockSplit(block, size);
            removeBlock(allocated_block);
            return (void*)((char*)allocated_block + MEMBLOCK_HEADER_SIZE);
        // if block size is large enough to fit the request size
        } else if (block->size >= size) {
            removeBlock(block);
            return (void*)((char*)block + MEMBLOCK_HEADER_SIZE);
        } else if (checkCoalesceSize(block) >= (size + SMALLEST_MEMBLOCK_SIZE)) {
            struct blockHeader* allocated_block = blockSplit(blockCoalesce(block), size);
            removeBlock(allocated_block);
            return (void*)((char*)allocated_block + MEMBLOCK_HEADER_SIZE);
        } else if (checkCoalesceSize(block) >= size) {
            struct blockHeader* allocated_block = blockCoalesce(block);
            removeBlock(allocated_block);
            return (void*)((char*)allocated_block + MEMBLOCK_HEADER_SIZE);
        }
        block = block->next;
    }
    
    // if no suitable block found, request more memory from the system, split and return block
    mapMoreMemory();
    block = freelist;
    blockSplit(block, size);
    removeBlock(block);
    
    return (void*)((char*)block + MEMBLOCK_HEADER_SIZE);
}

// deallocate memory back into the free list
void yfree(void* block) {
    struct blockHeader* block_header = (void*)((char*)block - MEMBLOCK_HEADER_SIZE);
    block_header->next = freelist;
    freelist = block_header;
    if (block_header->next != NULL) block_header->next->prev = block_header;

    // change block free status in header and footer
    struct blockFooter* block_footer = (void*)((char*)block_header + MEMBLOCK_HEADER_SIZE + block_header->size);
    block_footer->free = true;
    block_header->free = block_footer->free;
}