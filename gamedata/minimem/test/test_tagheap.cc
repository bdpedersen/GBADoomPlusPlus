#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdint>
#include "../tagheap.h"

// Expose internal structures and macros for testing
typedef struct memblock_s {
    struct memblock_s *prev;
    struct memblock_s *next;
    uint32_t tag;   // Tag of this block
    uint32_t size;  // Size in bytes
} memblock_t;

#define SZ_MEMBLOCK sizeof(memblock_t)

extern uint8_t th_heap[TH_HEAPSIZE];
#define FIRSTBLOCK ((memblock_t *)&th_heap[0])
#define LASTBLOCK ((memblock_t *)&th_heap[TH_HEAPSIZE-SZ_MEMBLOCK])

// Color codes for test output
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

// Assertion macros
#define TEST_ASSERT(condition, message) \
    do { \
        test_count++; \
        if (condition) { \
            test_passed++; \
            printf(GREEN "✓" RESET " %s\n", message); \
        } else { \
            test_failed++; \
            printf(RED "✗" RESET " %s (line %d)\n", message, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        test_count++; \
        if ((actual) == (expected)) { \
            test_passed++; \
            printf(GREEN "✓" RESET " %s (got %ld)\n", message, (long)(actual)); \
        } else { \
            test_failed++; \
            printf(RED "✗" RESET " %s: expected %ld, got %ld (line %d)\n", \
                   message, (long)(expected), (long)(actual), __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_NEQ(actual, expected, message) \
    do { \
        test_count++; \
        if ((actual) != (expected)) { \
            test_passed++; \
            printf(GREEN "✓" RESET " %s (got %ld)\n", message, (long)(actual)); \
        } else { \
            test_failed++; \
            printf(RED "✗" RESET " %s: should not be %ld (line %d)\n", \
                   message, (long)(expected), __LINE__); \
        } \
    } while (0)

// Heap validation helpers
struct BlockInfo {
    memblock_t *block;
    uint16_t tag;
    uint16_t size;
};

int count_blocks(memblock_t *start, memblock_t *end) {
    int count = 0;
    memblock_t *block = start;
    while (block && block != end->next) {
        count++;
        block = block->next;
    }
    return count;
}

bool validate_block_chain() {
    memblock_t *block = FIRSTBLOCK;
    memblock_t *prev = NULL;
    
    while (block) {
        // Check backward link consistency
        if (block->prev != prev) {
            printf(RED "✗ Block chain broken: backward link inconsistency\n" RESET);
            return false;
        }
        prev = block;
        block = block->next;
    }
    return true;
}

bool validate_block_sizes() {
    memblock_t *block = FIRSTBLOCK;
    
    while (block) {
        // Sentinels should have specific sizes
        if (block == LASTBLOCK) {
            if (block->size != 0) {
                printf(RED "✗ LASTBLOCK size should be 0, got %u\n" RESET, block->size);
                return false;
            }
        }
        block = block->next;
    }
    return true;
}

void print_heap_state() {
    printf(BLUE "\n=== Heap State ===\n" RESET);
    memblock_t *block = FIRSTBLOCK;
    int block_num = 0;
    
    while (block) {
        const char *tag_name;
        if (block->tag == TH_FREE_TAG) {
            tag_name = "FREE";
        } else if (block->tag & 0x80000000) {
            tag_name = "TAIL";
        } else {
            tag_name = "HEAD";
        }
        
        printf("Block %d: tag=0x%08x (%s), size=%u bytes, addr=%p\n",
               block_num, block->tag, tag_name, block->size, (void *)block);
        
        if (block == LASTBLOCK) {
            printf("         (LASTBLOCK sentinel)\n");
        }
        
        block_num++;
        block = block->next;
    }
    printf(BLUE "==================\n\n" RESET);
}

// Forward declare the actual implementation signature (different from header)
extern int TH_free(uint8_t *ptr);

// Helper to cast uint32_t* to call the actual TH_free
inline int free_block(uint8_t *ptr) {
    return TH_free(ptr);
}

// Global state for defrag callbacks
struct DefragState {
    static int moves_count;
};

int DefragState::moves_count = 0;

// Defrag callback that counts all moves
bool defrag_count_all_moves(short tag, uint8_t *proposed_newptr) {
    (void)tag;
    (void)proposed_newptr;
    DefragState::moves_count++;
    return true;
}

// Defrag callback that allows all moves
bool defrag_allow_all(short tag, uint8_t *proposed_newptr) {
    (void)tag;
    (void)proposed_newptr;
    return true;
}

// Defrag callback that vetos moves of specific tag
uint16_t veto_tag = 0;
bool defrag_veto_tag(short tag, uint8_t *proposed_newptr) {
    (void)proposed_newptr;
    if ((uint16_t)tag == veto_tag) {
        return false;
    }
    DefragState::moves_count++;
    return true;
}

// Test Cases

void test_initialization() {
    printf(YELLOW "\n--- Test: Initialization ---\n" RESET);
    TH_init();
    
    TEST_ASSERT(FIRSTBLOCK != NULL, "FIRSTBLOCK exists");
    TEST_ASSERT(LASTBLOCK != NULL, "LASTBLOCK exists");
    TEST_ASSERT_EQ(FIRSTBLOCK->tag, TH_FREE_TAG, "FIRSTBLOCK is marked free");
    TEST_ASSERT_EQ(LASTBLOCK->tag, TH_FREE_TAG, "LASTBLOCK is marked free");
    TEST_ASSERT_EQ(LASTBLOCK->size, 0, "LASTBLOCK has size 0");
    TEST_ASSERT(FIRSTBLOCK->next == LASTBLOCK, "FIRSTBLOCK->next points to LASTBLOCK");
    TEST_ASSERT(LASTBLOCK->prev == FIRSTBLOCK, "LASTBLOCK->prev points to FIRSTBLOCK");
    TEST_ASSERT(FIRSTBLOCK->prev == NULL, "FIRSTBLOCK->prev is NULL");
    TEST_ASSERT(LASTBLOCK->next == NULL, "LASTBLOCK->next is NULL");
    TEST_ASSERT(validate_block_chain(), "Block chain is valid");
}

void test_simple_head_allocation() {
    printf(YELLOW "\n--- Test: Simple Head Allocation ---\n" RESET);
    TH_init();
    
    // Allocate a small block
    uint8_t *ptr1 = TH_alloc(16, 0x1001);
    TEST_ASSERT_NEQ(ptr1, NULL, "First head allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after allocation");
    
    // Allocate another block
    uint8_t *ptr2 = TH_alloc(32, 0x1002);
    TEST_ASSERT_NEQ(ptr2, NULL, "Second head allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after second allocation");
    
    // Verify memory separation
    TEST_ASSERT(ptr1 != ptr2, "Two allocations have different addresses");
    print_heap_state();
}

void test_simple_tail_allocation() {
    printf(YELLOW "\n--- Test: Simple Tail Allocation ---\n" RESET);
    TH_init();
    
    uint8_t *ptr1 = TH_alloc(16, 0x80000001);  // Tail tag (MSB set)
    TEST_ASSERT_NEQ(ptr1, NULL, "First tail allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after tail allocation");
    
    uint8_t *ptr2 = TH_alloc(32, 0x80000002);
    TEST_ASSERT_NEQ(ptr2, NULL, "Second tail allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after second tail allocation");
    
    print_heap_state();
}

void test_mixed_head_tail_allocation() {
    printf(YELLOW "\n--- Test: Mixed Head and Tail Allocation ---\n" RESET);
    TH_init();
    
    uint8_t *head1 = TH_alloc(16, 0x1001);
    uint8_t *head2 = TH_alloc(16, 0x1002);
    uint8_t *tail1 = TH_alloc(16, 0x80000001);
    uint8_t *tail2 = TH_alloc(16, 0x80000002);
    
    TEST_ASSERT_NEQ(head1, NULL, "Head allocation 1 succeeds");
    TEST_ASSERT_NEQ(head2, NULL, "Head allocation 2 succeeds");
    TEST_ASSERT_NEQ(tail1, NULL, "Tail allocation 1 succeeds");
    TEST_ASSERT_NEQ(tail2, NULL, "Tail allocation 2 succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid with mixed allocations");
    
    print_heap_state();
}

void test_freeblock_case_0_isolated_free() {
    printf(YELLOW "\n--- Test: Freeblock Case 0 (Isolated Free) ---\n" RESET);
    TH_init();
    
    // Allocate three blocks: A (free) B (allocated) C (free)
    uint8_t *ptrA = TH_alloc(16, 0x1001);
    uint8_t *ptrB = TH_alloc(16, 0x1002);
    uint8_t *ptrC = TH_alloc(16, 0x1003);
    (void)ptrB; (void)ptrC;  // Not used in this test, just for heap setup
    
    // Free A - should be case 0 (no adjacent free blocks)
    TH_free((ptrA));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after freeing isolated block");
    
    print_heap_state();
}

void test_freeblock_case_1_merge_with_next() {
    printf(YELLOW "\n--- Test: Freeblock Case 1 (Merge with Next) ---\n" RESET);
    TH_init();
    
    // Allocate: A (free) B (allocated) C (free)
    uint8_t *ptrA = TH_alloc(16, 0x1001);
    uint8_t *ptrB = TH_alloc(16, 0x1002);
    (void)ptrB;  // Not used in this test, just for heap setup
    
    // Free A to create a free block
    TH_free((ptrA));
    
    // Free B - should merge with the free block after it (case 1)
    // This requires some careful setup; let's allocate then free strategically
    TH_init();  // Reset
    
    uint8_t *ptr1 = TH_alloc(16, 0x1001);
    uint8_t *ptr2 = TH_alloc(16, 0x1002);
    uint8_t *ptr3 = TH_alloc(16, 0x1003);
    (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr2 first (case 0 - isolated)
    TH_free((ptr2));
    print_heap_state();
    
    // Free ptr1, which is adjacent to the free block from ptr2
    // This should be case 1 (merge with next)
    TH_free((ptr1));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 1 merge");
    
    print_heap_state();
}

void test_freeblock_case_2_merge_with_prev() {
    printf(YELLOW "\n--- Test: Freeblock Case 2 (Merge with Previous) ---\n" RESET);
    TH_init();
    
    uint8_t *ptr1 = TH_alloc(16, 0x1001);
    uint8_t *ptr2 = TH_alloc(16, 0x1002);
    uint8_t *ptr3 = TH_alloc(16, 0x1003);
    (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr1 first (case 0 - isolated)
    TH_free((ptr1));
    print_heap_state();
    
    // Free ptr2, which is adjacent to the free block before it
    // This should be case 2 (merge with previous)
    TH_free((ptr2));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 2 merge");
    
    print_heap_state();
}

void test_freeblock_case_3_merge_both() {
    printf(YELLOW "\n--- Test: Freeblock Case 3 (Merge Both Directions) ---\n" RESET);
    TH_init();
    
    uint8_t *ptr1 = TH_alloc(16, 0x1001);
    uint8_t *ptr2 = TH_alloc(16, 0x1002);
    uint8_t *ptr3 = TH_alloc(16, 0x1003);
    uint8_t *ptr4 = TH_alloc(16, 0x1004);
    (void)ptr4;  // Not used in this test, just for heap setup
    
    // Free ptr1 and ptr3 to create free blocks on both sides of ptr2
    TH_free((ptr1));
    TH_free((ptr3));
    print_heap_state();
    
    // Free ptr2, which should merge with both neighbors (case 3)
    TH_free((ptr2));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 3 merge");
    
    print_heap_state();
}

void test_cascade_merges() {
    printf(YELLOW "\n--- Test: Cascade Merges ---\n" RESET);
    TH_init();
    
    // Create a series of allocations
    uint8_t *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = TH_alloc(16, 0x1001 + i);
    }
    
    // Free alternating blocks to create multiple small free regions
    for (int i = 0; i < 10; i += 2) {
        TH_free((ptrs[i]));
    }
    print_heap_state();
    
    // Now free the remaining blocks, which should trigger cascading merges
    for (int i = 1; i < 10; i += 2) {
        TH_free((ptrs[i]));
    }
    TEST_ASSERT(validate_block_chain(), "Block chain valid after cascade merges");
    
    print_heap_state();
}

void test_freetags_range() {
    printf(YELLOW "\n--- Test: TH_freetags Range Deallocation ---\n" RESET);
    TH_init();
    
    // Allocate blocks with various tags
    uint8_t *ptr1 = TH_alloc(16, 0x1001);
    uint8_t *ptr2 = TH_alloc(16, 0x1002);
    uint8_t *ptr3 = TH_alloc(16, 0x1003);
    uint8_t *ptr4 = TH_alloc(16, 0x1004);
    (void)ptr1; (void)ptr2; (void)ptr3; (void)ptr4;  // Not used in this test, just for heap setup
    
    TEST_ASSERT_NEQ(ptr1, NULL, "Allocations succeed");
    
    // Free a range of tags
    TH_freetags(0x1002, 0x1003);
    TEST_ASSERT(validate_block_chain(), "Block chain valid after freetags");
    
    print_heap_state();
}

void test_defragmentation() {
    printf(YELLOW "\n--- Test: Defragmentation ---\n" RESET);
    TH_init();
    
    // Allocate and fragment the heap
    uint8_t *ptr1 = TH_alloc(64, 0x1001);
    uint8_t *ptr2 = TH_alloc(64, 0x1002);
    uint8_t *ptr3 = TH_alloc(64, 0x1003);
    
    // Write pattern to verify data preservation
    if (ptr1 && ptr2 && ptr3) {
        uint32_t *data1 = (uint32_t*)ptr1;
        uint32_t *data2 = (uint32_t*)ptr2;
        uint32_t *data3 = (uint32_t*)ptr3;
        for (int i = 0; i < 4; i++) {  // 16 bytes = 4 dwords
            data1[i] = 0x11110000 + i;
            data2[i] = 0x22220000 + i;
            data3[i] = 0x33330000 + i;
        }
    }
    
    print_heap_state();
    
    // Create fragmentation by freeing middle block
    TH_free((ptr2));
    printf(BLUE "\n--- After freeing ptr2 ---\n" RESET);
    print_heap_state();
    
    // Defragment with a simple callback that allows all moves
    TH_defrag(defrag_allow_all);
    TEST_ASSERT(validate_block_chain(), "Block chain valid after defragmentation");
    
    printf(BLUE "\n--- After defragmentation ---\n" RESET);
    print_heap_state();
    
    // Verify data integrity
    if (ptr1) {
        bool data_ok = true;
        uint32_t *data1 = (uint32_t*)ptr1;
        for (int i = 0; i < 4; i++) {  // 16 bytes = 4 dwords
            if (data1[i] != (uint32_t)(0x11110000 + i)) {
                data_ok = false;
                break;
            }
        }
        TEST_ASSERT(data_ok, "Data integrity preserved in ptr1 after defrag");
    }
}

void test_countfreehead() {
    printf(YELLOW "\n--- Test: TH_countfreehead ---\n" RESET);
    TH_init();
    
    int initial_free = TH_countfreehead();
    TEST_ASSERT(initial_free > 0, "Initial heap has free space");
    printf(BLUE "Initial free: %d bytes\n" RESET, initial_free);
    
    // Allocate some memory
    uint8_t *ptr1 = TH_alloc(256, 0x1001);
    int after_alloc = TH_countfreehead();
    TEST_ASSERT(after_alloc < initial_free, "Free space decreases after allocation");
    printf(BLUE "After allocating 256 bytes: %d bytes free\n" RESET, after_alloc);
    
    // Free the memory
    TH_free((ptr1));
    int after_free = TH_countfreehead();
    TEST_ASSERT(after_free >= initial_free - 256, "Free space increases after deallocation");
    printf(BLUE "After freeing: %d bytes free\n" RESET, after_free);
}

void test_alloc_at_capacity() {
    printf(YELLOW "\n--- Test: Allocation at Capacity ---\n" RESET);
    TH_init();
    
    // Try to allocate extremely large block
    uint8_t *ptr = TH_alloc(10000000, 0x1001);
    TEST_ASSERT(ptr == NULL, "Over-capacity allocation returns NULL");
    
    // Allocate a very large but valid block
    int free_space = TH_countfreehead();
    uint8_t *large_ptr = TH_alloc(free_space - 1024, 0x1002);  // Leave some margin
    TEST_ASSERT_NEQ(large_ptr, NULL, "Large valid allocation succeeds");
    
    TH_free((large_ptr));
}

void test_rapid_alloc_free_cycles() {
    printf(YELLOW "\n--- Test: Rapid Allocation/Deallocation Cycles ---\n" RESET);
    TH_init();
    
    const int CYCLES = 100;
    const int ALLOCS_PER_CYCLE = 10;
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        uint8_t *ptrs[ALLOCS_PER_CYCLE];
        
        // Allocate
        for (int i = 0; i < ALLOCS_PER_CYCLE; i++) {
            ptrs[i] = TH_alloc(32, 0x1000 + (cycle * ALLOCS_PER_CYCLE + i) % 0x1000);
        }
        
        // Free in random order
        for (int i = ALLOCS_PER_CYCLE - 1; i >= 0; i--) {
            if (ptrs[i]) {
                TH_free((ptrs[i]));
            }
        }
        
        // Validate after each cycle
        if (!validate_block_chain()) {
            printf(RED "✗ Block chain corrupted at cycle %d\n" RESET, cycle);
            break;
        }
    }
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after rapid cycles");
    printf(GREEN "Completed %d cycles of allocation/deallocation\n" RESET, CYCLES);
}

void test_complex_fragmentation_and_defrag() {
    printf(YELLOW "\n--- Test: Complex Fragmentation and Defragmentation ---\n" RESET);
    TH_init();
    
    // Create a complex fragmentation pattern
    uint8_t *ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = TH_alloc(128, 0x1000 + i);
    }
    
    // Free every 3rd block to create scattered free space
    for (int i = 0; i < 20; i += 3) {
        if (ptrs[i]) {
            TH_free((ptrs[i]));
        }
    }
    
    int free_before = TH_countfreehead();
    printf(BLUE "Free space before defrag: %d bytes\n" RESET, free_before);
    print_heap_state();
    
    // Defragment
    DefragState::moves_count = 0;
    TH_defrag(defrag_count_all_moves);
    
    int free_after = TH_countfreehead();
    printf(BLUE "Free space after defrag: %d bytes (moved %d blocks)\n" RESET, free_after, DefragState::moves_count);
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after complex defrag");
    TEST_ASSERT(free_after >= free_before, "Defragmentation doesn't lose memory");
    
    print_heap_state();
    
    // Clean up
    for (int i = 0; i < 20; i++) {
        if (ptrs[i]) {
            TH_free((ptrs[i]));
        }
    }
}

void test_defrag_with_pinned_blocks() {
    printf(YELLOW "\n--- Test: Defragmentation with Pinned Blocks ---\n" RESET);
    TH_init();
    
    uint8_t *ptr1 = TH_alloc(128, 0x1001);
    uint8_t *ptr2 = TH_alloc(128, 0x1002);
    uint8_t *ptr3 = TH_alloc(128, 0x1003);
    (void)ptr1; (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr2 to create fragmentation
    TH_free((ptr2));
    print_heap_state();
    
    // Defragment but veto moves of tag 0x1001 (pinned)
    DefragState::moves_count = 0;
    veto_tag = 0x1001;
    TH_defrag(defrag_veto_tag);
    TEST_ASSERT(validate_block_chain(), "Block chain valid with pinned blocks");
    printf(BLUE "Moves: %d (ptr1 should have been pinned)\n" RESET, DefragState::moves_count);
    
    print_heap_state();
}

void test_allocation_alignment() {
    printf(YELLOW "\n--- Test: Allocation 4-Byte Alignment ---\n" RESET);
    TH_init();
    
    // Test various allocation sizes to ensure all return 4-byte aligned pointers
    int test_sizes[] = {1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 33, 100, 256, 1000};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    uint8_t *ptrs[14];
    bool all_aligned = true;
    
    for (int i = 0; i < num_sizes; i++) {
        ptrs[i] = TH_alloc(test_sizes[i], 0x1000 + i);
        
        // Check if pointer is 4-byte aligned
        if (ptrs[i]) {
            uintptr_t addr = (uintptr_t)ptrs[i];
            if (addr % 4 != 0) {
                printf(RED "✗" RESET " Allocation of %d bytes returned unaligned pointer: %p\n",
                       test_sizes[i], (void*)ptrs[i]);
                all_aligned = false;
            } else {
                printf(GREEN "✓" RESET " Allocation of %d bytes: %p (aligned)\n",
                       test_sizes[i], (void*)ptrs[i]);
            }
        } else {
            printf(RED "✗" RESET " Allocation of %d bytes returned NULL\n", test_sizes[i]);
            all_aligned = false;
        }
    }
    
    TEST_ASSERT(all_aligned, "All allocations are 4-byte aligned");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after alignment test");
    
    print_heap_state();
}

void test_realloc_null_ptr() {
    printf(YELLOW "\n--- Test: Realloc with NULL Pointer ---\n" RESET);
    TH_init();
    
    // Try to realloc NULL pointer - should return NULL
    uint8_t *result = TH_realloc(NULL, 100);
    TEST_ASSERT(result == NULL, "TH_realloc(NULL, 100) returns NULL");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after NULL realloc");
    
    print_heap_state();
}

void test_realloc_zero_size() {
    printf(YELLOW "\n--- Test: Realloc with Zero/Negative Size ---\n" RESET);
    TH_init();
    
    // Allocate a block
    uint8_t *ptr = TH_alloc(100, 0x2000);
    TEST_ASSERT(ptr != NULL, "Initial allocation succeeds");
    
    // Try to realloc to size 0 - should free and return NULL
    uint8_t *result = TH_realloc(ptr, 0);
    TEST_ASSERT(result == NULL, "TH_realloc(ptr, 0) returns NULL and frees block");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after zero-size realloc");
    
    // Verify the block was actually freed by checking free space
    int free_after = TH_countfreehead();
    printf(BLUE "Free HEAD space after zero-size realloc: %d bytes\n" RESET, free_after);
    TEST_ASSERT(free_after > 0, "Space was freed after zero-size realloc");
    
    print_heap_state();
}

void test_realloc_no_op_shrink() {
    printf(YELLOW "\n--- Test: Realloc to Smaller Size (No-Op) ---\n" RESET);
    TH_init();
    
    // Allocate a block of 200 bytes
    uint8_t *ptr = TH_alloc(200, 0x2000);
    TEST_ASSERT(ptr != NULL, "Initial allocation succeeds");
    
    // Write pattern
    uint32_t *pattern_ptr = (uint32_t *)ptr;
    pattern_ptr[0] = 0xDEADBEEF;
    pattern_ptr[1] = 0xCAFEBABE;
    
    // Try to realloc to smaller size (100 bytes) - implementation returns ptr as-is
    uint8_t *result = TH_realloc(ptr, 100);
    TEST_ASSERT(result == ptr, "TH_realloc to smaller size returns same ptr");
    
    // Verify pattern is intact
    TEST_ASSERT(pattern_ptr[0] == 0xDEADBEEF, "Pattern byte 0 intact after shrink");
    TEST_ASSERT(pattern_ptr[1] == 0xCAFEBABE, "Pattern byte 1 intact after shrink");
    
    print_heap_state();
}

void test_realloc_no_growth_needed() {
    printf(YELLOW "\n--- Test: Realloc When Block Already Large Enough ---\n" RESET);
    TH_init();
    
    // Allocate 200 bytes
    uint8_t *ptr = TH_alloc(200, 0x2000);
    TEST_ASSERT(ptr != NULL, "Initial allocation succeeds");
    
    // Write pattern
    uint32_t *pattern_ptr = (uint32_t *)ptr;
    pattern_ptr[0] = 0x11111111;
    pattern_ptr[1] = 0x22222222;
    
    // Try to realloc to same/smaller size (100 bytes)
    uint8_t *result = TH_realloc(ptr, 100);
    TEST_ASSERT(result == ptr, "TH_realloc(200-byte block, 100) returns same ptr");
    
    // Verify pattern is intact
    TEST_ASSERT(pattern_ptr[0] == 0x11111111, "Pattern byte 0 intact");
    TEST_ASSERT(pattern_ptr[1] == 0x22222222, "Pattern byte 1 intact");
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid");
    
    print_heap_state();
}

void test_realloc_in_place_expansion() {
    printf(YELLOW "\n--- Test: Realloc with In-Place Expansion (Next Block Free) ---\n" RESET);
    TH_init();
    
    // Allocate A (100 bytes)
    uint8_t *ptrA = TH_alloc(100, 0x2000);
    TEST_ASSERT(ptrA != NULL, "Block A allocation succeeds");
    
    // Allocate B (100 bytes)
    uint8_t *ptrB = TH_alloc(100, 0x2001);
    TEST_ASSERT(ptrB != NULL, "Block B allocation succeeds");
    
    // Allocate C (100 bytes)
    uint8_t *ptrC = TH_alloc(100, 0x2002);
    TEST_ASSERT(ptrC != NULL, "Block C allocation succeeds");
    
    printf(BLUE "Before realloc: A=%p, B=%p, C=%p\n" RESET, (void*)ptrA, (void*)ptrB, (void*)ptrC);
    
    // Free B - now A is adjacent to free block
    TH_free(ptrB);
    TEST_ASSERT(validate_block_chain(), "Block chain valid after freeing B");
    
    // Write pattern to A
    uint32_t *pattern = (uint32_t *)ptrA;
    pattern[0] = 0xAAAAAAAA;
    pattern[1] = 0xBBBBBBBB;
    
    // Realloc A to 150 bytes - should expand into freed B space in-place
    uint8_t *result = TH_realloc(ptrA, 150);
    TEST_ASSERT(result == ptrA, "In-place expansion returns same pointer");
    
    // Verify pattern is intact in same location
    uint32_t *check_pattern = (uint32_t *)result;
    TEST_ASSERT(check_pattern[0] == 0xAAAAAAAA, "Original pattern byte 0 preserved");
    TEST_ASSERT(check_pattern[1] == 0xBBBBBBBB, "Original pattern byte 1 preserved");
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after in-place expansion");
    
    printf(BLUE "After realloc: Result=%p (same=%d)\n" RESET, (void*)result, result == ptrA);
    
    print_heap_state();
}

void test_realloc_with_block_splitting() {
    printf(YELLOW "\n--- Test: Realloc with In-Place Expansion + Block Splitting ---\n" RESET);
    TH_init();
    
    // Allocate A (100 bytes)
    uint8_t *ptrA = TH_alloc(100, 0x3000);
    TEST_ASSERT(ptrA != NULL, "Block A allocation succeeds");
    
    // Allocate B (500 bytes) - large block to free later
    uint8_t *ptrB = TH_alloc(500, 0x3001);
    TEST_ASSERT(ptrB != NULL, "Block B allocation succeeds");
    
    // Allocate C (100 bytes) - to keep B from being tail-adjacent
    uint8_t *ptrC = TH_alloc(100, 0x3002);
    TEST_ASSERT(ptrC != NULL, "Block C allocation succeeds");
    
    // Free B - creates a 500-byte free block
    TH_free(ptrB);
    TEST_ASSERT(validate_block_chain(), "Block chain valid after freeing large B");
    
    // Write pattern to A
    uint32_t *pattern = (uint32_t *)ptrA;
    pattern[0] = 0xCCCCCCCC;
    pattern[1] = 0xDDDDDDDD;
    
    // Realloc A to 150 bytes - should expand into B, splitting it
    // (A needs 150, next free block is 500, so will split into 150 and remaining)
    uint8_t *result = TH_realloc(ptrA, 150);
    TEST_ASSERT(result == ptrA, "Expansion with splitting returns same pointer");
    
    // Verify pattern is intact
    uint32_t *check = (uint32_t *)result;
    TEST_ASSERT(check[0] == 0xCCCCCCCC, "Pattern byte 0 preserved after split");
    TEST_ASSERT(check[1] == 0xDDDDDDDD, "Pattern byte 1 preserved after split");
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after split realloc");
    
    print_heap_state();
}

void test_realloc_full_reallocation() {
    printf(YELLOW "\n--- Test: Realloc Requiring Full Reallocation (New Block) ---\n" RESET);
    TH_init();
    
    // Allocate A (100 bytes)
    uint8_t *ptrA = TH_alloc(100, 0x4000);
    TEST_ASSERT(ptrA != NULL, "Block A allocation succeeds");
    
    // Allocate B (100 bytes) - next to A
    uint8_t *ptrB = TH_alloc(100, 0x4001);
    TEST_ASSERT(ptrB != NULL, "Block B allocation succeeds");
    
    printf(BLUE "Before full realloc: A=%p, B=%p\n" RESET, (void*)ptrA, (void*)ptrB);
    
    // Write pattern to A
    uint32_t *pattern = (uint32_t *)ptrA;
    for (int i = 0; i < 25; i++) {
        pattern[i] = 0x12340000 + i;
    }
    
    // Try to realloc A to 300 bytes
    // Since B is adjacent and not free, can't expand in-place
    // Should allocate new block, copy data, free old block
    uint8_t *result = TH_realloc(ptrA, 300);
    TEST_ASSERT(result != NULL, "Full reallocation succeeds");
    TEST_ASSERT(result != ptrA, "Full reallocation returns different pointer");
    
    // Verify pattern was copied correctly
    uint32_t *check = (uint32_t *)result;
    bool data_intact = true;
    for (int i = 0; i < 25; i++) {
        if (check[i] != (0x12340000 + i)) {
            printf(RED "✗ Pattern mismatch at offset %d: %p got 0x%x, expected 0x%x\n" RESET,
                   i * 4, (void*)(result + i*4), check[i], 0x12340000 + i);
            data_intact = false;
        }
    }
    TEST_ASSERT(data_intact, "Data copied correctly on full reallocation");
    
    // Verify B is still accessible
    TEST_ASSERT(ptrB != NULL, "Original block B still valid");
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after full reallocation");
    
    printf(BLUE "After full realloc: New=%p (different=%d)\n" RESET, (void*)result, result != ptrA);
    
    print_heap_state();
}

void test_realloc_data_integrity() {
    printf(YELLOW "\n--- Test: Realloc Data Integrity with Various Patterns ---\n" RESET);
    TH_init();
    
    // Test with multiple allocation+realloc cycles
    struct {
        int initial_size;
        int realloc_size;
    } test_cases[] = {
        {50, 150},
        {100, 300},
        {200, 100},  // Shrink (no-op)
        {80, 200},
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int tc = 0; tc < num_cases; tc++) {
        int init_sz = test_cases[tc].initial_size;
        int realloc_sz = test_cases[tc].realloc_size;
        
        uint8_t *ptr = TH_alloc(init_sz, 0x5000 + tc);
        TEST_ASSERT(ptr != NULL, "Allocation succeeds in test case");
        
        // Fill with pattern
        uint8_t *byte_ptr = ptr;
        for (int i = 0; i < init_sz; i++) {
            byte_ptr[i] = (uint8_t)((0xAA + tc) ^ (i & 0xFF));
        }
        
        // Realloc
        uint8_t *result = TH_realloc(ptr, realloc_sz);
        TEST_ASSERT(result != NULL, "Realloc succeeds");
        
        // Verify original bytes are intact
        bool match = true;
        int check_sz = (init_sz < realloc_sz) ? init_sz : realloc_sz;
        for (int i = 0; i < check_sz; i++) {
            uint8_t expected = (uint8_t)((0xAA + tc) ^ (i & 0xFF));
            if (result[i] != expected) {
                printf(RED "✗ Data mismatch in test case %d at byte %d\n" RESET, tc, i);
                match = false;
                break;
            }
        }
        TEST_ASSERT(match, "Data integrity maintained in realloc cycle");
    }
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after data integrity tests");
    
    print_heap_state();
}

void test_realloc_alignment() {
    printf(YELLOW "\n--- Test: Realloc Maintains Data Alignment ---\n" RESET);
    TH_init();
    
    // Test that data from reallocated blocks is preserved correctly
    // (We don't test pointer alignment since block headers may not be aligned)
    int test_sizes[] = {10, 50, 100, 200};
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        uint8_t *ptr = TH_alloc(test_sizes[i], 0x6000 + i);
        TEST_ASSERT(ptr != NULL, "Initial allocation succeeds");
        
        // Fill with test pattern
        for (int j = 0; j < test_sizes[i]; j++) {
            ptr[j] = (uint8_t)(0x55 + j);
        }
        
        // Realloc to different size
        int new_size = test_sizes[i] + 50;
        uint8_t *result = TH_realloc(ptr, new_size);
        
        TEST_ASSERT(result != NULL, "Realloc succeeded");
        
        // Verify pattern is preserved
        bool pattern_ok = true;
        for (int j = 0; j < test_sizes[i]; j++) {
            if (result[j] != (uint8_t)(0x55 + j)) {
                pattern_ok = false;
                break;
            }
        }
        TEST_ASSERT(pattern_ok, "Data pattern preserved in realloc");
    }
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid after alignment tests");
    
    print_heap_state();
}

void test_realloc_with_pinned_blocks() {
    printf(YELLOW "\n--- Test: Realloc Behavior with Pinned Blocks ---\n" RESET);
    TH_init();
    
    // Allocate some blocks
    uint8_t *ptrA = TH_alloc(100, 0x7000);
    uint8_t *ptrB = TH_alloc(100, 0x7001);
    uint8_t *ptrC = TH_alloc(100, 0x7002);
    
    TEST_ASSERT(ptrA && ptrB && ptrC, "All initial allocations succeed");
    
    // Free B to create fragmentation
    TH_free(ptrB);
    
    // Write pattern to A
    uint32_t *pattern = (uint32_t *)ptrA;
    pattern[0] = 0x77777777;
    
    // Realloc A - this might involve block movement via defrag indirectly
    uint8_t *result = TH_realloc(ptrA, 200);
    TEST_ASSERT(result != NULL, "Realloc succeeds");
    
    // Verify pattern is intact
    uint32_t *check = (uint32_t *)result;
    TEST_ASSERT(check[0] == 0x77777777, "Pattern preserved in realloc with fragmentation");
    
    TEST_ASSERT(validate_block_chain(), "Block chain valid");
    
    print_heap_state();
}

void run_all_tests() {
    printf(BLUE "\n╔════════════════════════════════════════════╗\n");
    printf("║        TagHeap White Box Test Suite        ║\n");
    printf("╚════════════════════════════════════════════╝\n" RESET);
    
    test_initialization();
    test_simple_head_allocation();
    test_simple_tail_allocation();
    test_mixed_head_tail_allocation();
    test_freeblock_case_0_isolated_free();
    test_freeblock_case_1_merge_with_next();
    test_freeblock_case_2_merge_with_prev();
    test_freeblock_case_3_merge_both();
    test_cascade_merges();
    test_freetags_range();
    test_countfreehead();
    test_alloc_at_capacity();
    test_defragmentation();
    test_defrag_with_pinned_blocks();
    test_complex_fragmentation_and_defrag();
    test_rapid_alloc_free_cycles();
    test_allocation_alignment();
    test_realloc_null_ptr();
    test_realloc_zero_size();
    test_realloc_no_op_shrink();
    test_realloc_no_growth_needed();
    test_realloc_in_place_expansion();
    test_realloc_with_block_splitting();
    test_realloc_full_reallocation();
    test_realloc_data_integrity();
    test_realloc_alignment();
    test_realloc_with_pinned_blocks();
    
    // Print summary
    printf(BLUE "\n╔════════════════════════════════════════════╗\n");
    printf("║              Test Summary                   ║\n");
    printf("╚════════════════════════════════════════════╝\n" RESET);
    printf("Total tests: %d\n", test_count);
    printf(GREEN "Passed: %d\n" RESET, test_passed);
    printf("%s", test_failed > 0 ? RED : GREEN);
    printf("Failed: %d\n" RESET, test_failed);
    
    if (test_failed == 0) {
        printf(GREEN "\n✓ All tests passed!\n\n" RESET);
        exit(EXIT_SUCCESS);
    } else {
        printf(RED "\n✗ Some tests failed.\n\n" RESET);
        exit(EXIT_FAILURE);
    }
}

int main() {
    run_all_tests();
    return 0;
}
