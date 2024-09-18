#include <math.h>
#include <stdbool.h>

#define FREELIST_AMOUNT 10
#define FREELIST_DISTRIBUTION_INITIAL 100
#define FREELIST_DISTRIBUTION_FACTOR 0.7
#define FREELIST_DISTRIBUTION_RATIO 0.7

#define PLATFORM_WORD_SIZE sizeof(void*)
#define BLOCK_ALIGNMENT_SIZE 2 * WORD_SIZE
#define BLOCK_METADATA_SIZE ((sizeof(blockMetadata) + BLOCK_ALIGNMENT_SIZE -1) & ~(BLOCK_ALIGNMENT_SIZE -1))
#define BLOCK_SMALLEST_SIZE ((2 * BLOCK_METADATA_SIZE) + BLOCK_ALIGNMENT_SIZE)

typedef struct {
    size_t size;
    bool free;
} blockMetadata;

typedef struct {
    blockMetadata* next_header;
    blockMetadata* prev_header;
} listPtrs;

static blockMetadata* free_lists[FREELIST_AMOUNT];

// allocate memory from the operating system
void* allocate_memory(size) {
    void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
}

// initialize an amount of contiguous blocks of certain size in memory
blockMetadata* initialize_blocks(blockMetadata* header, int block_size, int block_amount, bool link) {
        // initialize first block
        listPtrs* pointers = (void*)((char*)header + BLOCK_METADATA_SIZE)
        blockMetadata* footer = (void*)((char*)list_pointers + block_size)
        blockMetadata* next_header = (void*)((char*)footer + BLOCK_METADATA_SIZE);
        listPtrs* next_pointers = (void*)((char*)next_header + BLOCK_METADATA_SIZE)
        header->size = block_size;
        header->free = true;
        *footer = *header;
        pointers->next_header = next_header;
        next_pointers->prev_header = header;
        header = next_header;

    for (k=0; k < small_block_amount - 2; k++) {
        listPtrs* pointers = (void*)((char*)header + BLOCK_METADATA_SIZE)
        blockMetadata* footer = (void*)((char*)list_pointers + block_size)
        blockMetadata* next_header = (void*)((char*)footer + BLOCK_METADATA_SIZE);
        listPtrs* next_pointers = (void*)((char*)next_header + BLOCK_METADATA_SIZE)
        header->size = block_size;
        header->free = true;
        *footer = *header;
        pointers->next_header = next_header;
        next_pointers->prev_header = header;
        header = next_header;
    }

    // initialize last block

    return header;
}

void init() {
    
    // initialize each list
    for (i=0 ; i < FREELIST_AMOUNT; i++) {

        // calculate block amounts
        int block_amount = FREELIST_DISTRIBUTION_INITIAL * pow(0.7, i);
        int small_block_amount = block_amount * FREELIST_DISTRIBUTION_RATIO;
        int large_block_amount = block_amount - small_block_amount;
        
        // calculate block data sizes
        int small_block_data_size = BLOCK_ALIGNMENT_SIZE + (i * 2 * BLOCK_ALIGNMENT_SIZE);
        int large_block_data_size = small_block_data_size + BLOCK_ALIGNMENT_SIZE;
        
        // calculate memory requirement
        int list_memory_size = (small_block_amount * small_block_data_size) + (large_block_amount * large_block_data_size) + (block_amount * 2 * BLOCK_METADATA_SIZE);
        
        // allocate current list memory from the OS
        free_lists[i] = allocate_memory(list_memory_size);

        // initialize blocks
        initialize_blocks(initialize_blocks(free_lists[i], small_block_data_size, small_block_amount, true), large_block_data_size, large_block_amount, false)
    }
}


// print amount of blocks of each size and status
void print_block_stats() {
    for (i=0; i < FREELIST_AMOUNT; i++) {
        current_block = free_lists[i];
        current_
    }
}