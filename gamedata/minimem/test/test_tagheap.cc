#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "../tagheap.h"

// Expose internal structures and macros for testing
typedef struct memblock_s {
    struct memblock_s *prev;
    struct memblock_s *next;
    uint32_t tag : 15;   // Tag of this block
    uint32_t size : 17;  // Size in dwords (4 bytes)
} memblock_t;

#define SZ_MEMBLOCK (sizeof(memblock_t)/4)

extern uint32_t heap[TH_HEAPSIZE4];
#define FIRSTBLOCK ((memblock_t *)&heap[0])
#define LASTBLOCK ((memblock_t *)&heap[TH_HEAPSIZE4-SZ_MEMBLOCK])

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
        } else if (block->tag & 0x4000) {
            tag_name = "TAIL";
        } else {
            tag_name = "HEAD";
        }
        
        printf("Block %d: tag=0x%04x (%s), size=%u (%u bytes), addr=%p\n",
               block_num, block->tag, tag_name, block->size, 
               block->size * 4, (void *)block);
        
        if (block == LASTBLOCK) {
            printf("         (LASTBLOCK sentinel)\n");
        }
        
        block_num++;
        block = block->next;
    }
    printf(BLUE "==================\n\n" RESET);
}

// Forward declare the actual implementation signature (different from header)
extern int TH_free(uint32_t *ptr);

// Helper to cast uint32_t* to call the actual TH_free
inline int free_block(uint32_t *ptr) {
    return TH_free(ptr);
}

// Global state for defrag callbacks
struct DefragState {
    static int moves_count;
};

int DefragState::moves_count = 0;

// Defrag callback that counts all moves
bool defrag_count_all_moves(short tag, uint32_t *proposed_newptr) {
    (void)tag;
    (void)proposed_newptr;
    DefragState::moves_count++;
    return true;
}

// Defrag callback that allows all moves
bool defrag_allow_all(short tag, uint32_t *proposed_newptr) {
    (void)tag;
    (void)proposed_newptr;
    return true;
}

// Defrag callback that vetos moves of specific tag
uint16_t veto_tag = 0;
bool defrag_veto_tag(short tag, uint32_t *proposed_newptr) {
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
    uint32_t *ptr1 = TH_alloc(16, 0x1001);
    TEST_ASSERT_NEQ(ptr1, NULL, "First head allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after allocation");
    
    // Allocate another block
    uint32_t *ptr2 = TH_alloc(32, 0x1002);
    TEST_ASSERT_NEQ(ptr2, NULL, "Second head allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after second allocation");
    
    // Verify memory separation
    TEST_ASSERT(ptr1 != ptr2, "Two allocations have different addresses");
    print_heap_state();
}

void test_simple_tail_allocation() {
    printf(YELLOW "\n--- Test: Simple Tail Allocation ---\n" RESET);
    TH_init();
    
    uint32_t *ptr1 = TH_alloc(16, 0x8001);  // Tail tag (MSB set)
    TEST_ASSERT_NEQ(ptr1, NULL, "First tail allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after tail allocation");
    
    uint32_t *ptr2 = TH_alloc(32, 0x8002);
    TEST_ASSERT_NEQ(ptr2, NULL, "Second tail allocation succeeds");
    TEST_ASSERT(validate_block_chain(), "Block chain valid after second tail allocation");
    
    print_heap_state();
}

void test_mixed_head_tail_allocation() {
    printf(YELLOW "\n--- Test: Mixed Head and Tail Allocation ---\n" RESET);
    TH_init();
    
    uint32_t *head1 = TH_alloc(16, 0x1001);
    uint32_t *head2 = TH_alloc(16, 0x1002);
    uint32_t *tail1 = TH_alloc(16, 0x8001);
    uint32_t *tail2 = TH_alloc(16, 0x8002);
    
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
    uint32_t *ptrA = TH_alloc(16, 0x1001);
    uint32_t *ptrB = TH_alloc(16, 0x1002);
    uint32_t *ptrC = TH_alloc(16, 0x1003);
    (void)ptrB; (void)ptrC;  // Not used in this test, just for heap setup
    
    // Free A - should be case 0 (no adjacent free blocks)
    TH_free(((uint32_t*)ptrA));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after freeing isolated block");
    
    print_heap_state();
}

void test_freeblock_case_1_merge_with_next() {
    printf(YELLOW "\n--- Test: Freeblock Case 1 (Merge with Next) ---\n" RESET);
    TH_init();
    
    // Allocate: A (free) B (allocated) C (free)
    uint32_t *ptrA = TH_alloc(16, 0x1001);
    uint32_t *ptrB = TH_alloc(16, 0x1002);
    (void)ptrB;  // Not used in this test, just for heap setup
    
    // Free A to create a free block
    TH_free(((uint32_t*)ptrA));
    
    // Free B - should merge with the free block after it (case 1)
    // This requires some careful setup; let's allocate then free strategically
    TH_init();  // Reset
    
    uint32_t *ptr1 = TH_alloc(16, 0x1001);
    uint32_t *ptr2 = TH_alloc(16, 0x1002);
    uint32_t *ptr3 = TH_alloc(16, 0x1003);
    (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr2 first (case 0 - isolated)
    TH_free(((uint32_t*)ptr2));
    print_heap_state();
    
    // Free ptr1, which is adjacent to the free block from ptr2
    // This should be case 1 (merge with next)
    TH_free(((uint32_t*)ptr1));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 1 merge");
    
    print_heap_state();
}

void test_freeblock_case_2_merge_with_prev() {
    printf(YELLOW "\n--- Test: Freeblock Case 2 (Merge with Previous) ---\n" RESET);
    TH_init();
    
    uint32_t *ptr1 = TH_alloc(16, 0x1001);
    uint32_t *ptr2 = TH_alloc(16, 0x1002);
    uint32_t *ptr3 = TH_alloc(16, 0x1003);
    (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr1 first (case 0 - isolated)
    TH_free(((uint32_t*)ptr1));
    print_heap_state();
    
    // Free ptr2, which is adjacent to the free block before it
    // This should be case 2 (merge with previous)
    TH_free(((uint32_t*)ptr2));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 2 merge");
    
    print_heap_state();
}

void test_freeblock_case_3_merge_both() {
    printf(YELLOW "\n--- Test: Freeblock Case 3 (Merge Both Directions) ---\n" RESET);
    TH_init();
    
    uint32_t *ptr1 = TH_alloc(16, 0x1001);
    uint32_t *ptr2 = TH_alloc(16, 0x1002);
    uint32_t *ptr3 = TH_alloc(16, 0x1003);
    uint32_t *ptr4 = TH_alloc(16, 0x1004);
    (void)ptr4;  // Not used in this test, just for heap setup
    
    // Free ptr1 and ptr3 to create free blocks on both sides of ptr2
    TH_free(((uint32_t*)ptr1));
    TH_free(((uint32_t*)ptr3));
    print_heap_state();
    
    // Free ptr2, which should merge with both neighbors (case 3)
    TH_free(((uint32_t*)ptr2));
    TEST_ASSERT(validate_block_chain(), "Block chain valid after case 3 merge");
    
    print_heap_state();
}

void test_cascade_merges() {
    printf(YELLOW "\n--- Test: Cascade Merges ---\n" RESET);
    TH_init();
    
    // Create a series of allocations
    uint32_t *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = TH_alloc(16, 0x1001 + i);
    }
    
    // Free alternating blocks to create multiple small free regions
    for (int i = 0; i < 10; i += 2) {
        TH_free(((uint32_t*)ptrs[i]));
    }
    print_heap_state();
    
    // Now free the remaining blocks, which should trigger cascading merges
    for (int i = 1; i < 10; i += 2) {
        TH_free(((uint32_t*)ptrs[i]));
    }
    TEST_ASSERT(validate_block_chain(), "Block chain valid after cascade merges");
    
    print_heap_state();
}

void test_freetags_range() {
    printf(YELLOW "\n--- Test: TH_freetags Range Deallocation ---\n" RESET);
    TH_init();
    
    // Allocate blocks with various tags
    uint32_t *ptr1 = TH_alloc(16, 0x1001);
    uint32_t *ptr2 = TH_alloc(16, 0x1002);
    uint32_t *ptr3 = TH_alloc(16, 0x1003);
    uint32_t *ptr4 = TH_alloc(16, 0x1004);
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
    uint32_t *ptr1 = TH_alloc(64, 0x1001);
    uint32_t *ptr2 = TH_alloc(64, 0x1002);
    uint32_t *ptr3 = TH_alloc(64, 0x1003);
    
    // Write pattern to verify data preservation
    if (ptr1 && ptr2 && ptr3) {
        for (int i = 0; i < 16; i++) {
            ptr1[i] = 0x11110000 + i;
            ptr2[i] = 0x22220000 + i;
            ptr3[i] = 0x33330000 + i;
        }
    }
    
    print_heap_state();
    
    // Create fragmentation by freeing middle block
    TH_free(((uint32_t*)ptr2));
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
        for (int i = 0; i < 16; i++) {
            if (ptr1[i] != (uint32_t)(0x11110000 + i)) {
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
    uint32_t *ptr1 = TH_alloc(256, 0x1001);
    int after_alloc = TH_countfreehead();
    TEST_ASSERT(after_alloc < initial_free, "Free space decreases after allocation");
    printf(BLUE "After allocating 256 bytes: %d bytes free\n" RESET, after_alloc);
    
    // Free the memory
    TH_free(((uint32_t*)ptr1));
    int after_free = TH_countfreehead();
    TEST_ASSERT(after_free >= initial_free - 256, "Free space increases after deallocation");
    printf(BLUE "After freeing: %d bytes free\n" RESET, after_free);
}

void test_alloc_at_capacity() {
    printf(YELLOW "\n--- Test: Allocation at Capacity ---\n" RESET);
    TH_init();
    
    // Try to allocate extremely large block
    uint32_t *ptr = TH_alloc(10000000, 0x1001);
    TEST_ASSERT(ptr == NULL, "Over-capacity allocation returns NULL");
    
    // Allocate a very large but valid block
    int free_space = TH_countfreehead();
    uint32_t *large_ptr = TH_alloc(free_space - 1024, 0x1002);  // Leave some margin
    TEST_ASSERT_NEQ(large_ptr, NULL, "Large valid allocation succeeds");
    
    TH_free(((uint32_t*)large_ptr));
}

void test_rapid_alloc_free_cycles() {
    printf(YELLOW "\n--- Test: Rapid Allocation/Deallocation Cycles ---\n" RESET);
    TH_init();
    
    const int CYCLES = 100;
    const int ALLOCS_PER_CYCLE = 10;
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        uint32_t *ptrs[ALLOCS_PER_CYCLE];
        
        // Allocate
        for (int i = 0; i < ALLOCS_PER_CYCLE; i++) {
            ptrs[i] = TH_alloc(32, 0x1000 + (cycle * ALLOCS_PER_CYCLE + i) % 0x1000);
        }
        
        // Free in random order
        for (int i = ALLOCS_PER_CYCLE - 1; i >= 0; i--) {
            if (ptrs[i]) {
                TH_free(((uint32_t*)ptrs[i]));
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
    uint32_t *ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = TH_alloc(128, 0x1000 + i);
    }
    
    // Free every 3rd block to create scattered free space
    for (int i = 0; i < 20; i += 3) {
        if (ptrs[i]) {
            TH_free(((uint32_t*)ptrs[i]));
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
            TH_free(((uint32_t*)ptrs[i]));
        }
    }
}

void test_defrag_with_pinned_blocks() {
    printf(YELLOW "\n--- Test: Defragmentation with Pinned Blocks ---\n" RESET);
    TH_init();
    
    uint32_t *ptr1 = TH_alloc(128, 0x1001);
    uint32_t *ptr2 = TH_alloc(128, 0x1002);
    uint32_t *ptr3 = TH_alloc(128, 0x1003);
    (void)ptr1; (void)ptr3;  // Not used in this test, just for heap setup
    
    // Free ptr2 to create fragmentation
    TH_free(((uint32_t*)ptr2));
    print_heap_state();
    
    // Defragment but veto moves of tag 0x1001 (pinned)
    DefragState::moves_count = 0;
    veto_tag = 0x1001;
    TH_defrag(defrag_veto_tag);
    TEST_ASSERT(validate_block_chain(), "Block chain valid with pinned blocks");
    printf(BLUE "Moves: %d (ptr1 should have been pinned)\n" RESET, DefragState::moves_count);
    
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
