#include "guardmalloc.h"
// implementation of guardmalloc functions
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <set> 

struct AllocHeader {
    size_t size;
    const char* file;
    int line;
};

std::set<AllocHeader*> allocations;
std::set<AllocHeader*> freed_allocations;

void *gmalloc(size_t size, const char *file, int line) {
    size_t pagesize = sysconf(_SC_PAGESIZE);
    // Layout: [lower guard page] [header + user data rounded to page] [upper guard page]
    // This ensures the upper guard page is at a page-aligned address for mprotect
    size_t data_region_size = (sizeof(AllocHeader) + size + pagesize - 1) & ~(pagesize - 1);
    size_t total_size = pagesize + data_region_size + pagesize;

    // Allocate memory with mmap
    void *ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    // Set up guard pages (addresses MUST be page-aligned for mprotect)
    mprotect(ptr, pagesize, PROT_NONE);  // Lower guard page (starts at 0, page-aligned)
    mprotect((char*)ptr + pagesize + data_region_size, pagesize, PROT_NONE);  // Upper guard page (page-aligned by construction)

    // Store allocation header
    AllocHeader *header = (AllocHeader*)((char*)ptr + pagesize);
    header->size = size;
    header->file = file;
    header->line = line;

    // Track the allocation
    allocations.insert(header);

    return (char*)header + sizeof(AllocHeader);
}

void gfree(void *ptr, const char *file, int line) {
    if (!ptr) return;

    AllocHeader *header = (AllocHeader*)((char*)ptr - sizeof(AllocHeader));
    size_t pagesize = sysconf(_SC_PAGESIZE);
    // Data region size (header + user data) rounded up to page boundary
    size_t data_region_size = (header->size + sizeof(AllocHeader) + pagesize - 1) & ~(pagesize - 1);

    // Update header from where it was freed
    header->file = file;
    header->line = line;

    // Fill the rest of the memory with a pattern to make it easier to debug use-after-free
    memset((char*)header + sizeof(AllocHeader), 0xDE, header->size);

    // Protect the memory to catch use-after-free (protect data region)
    mprotect((char*)header, data_region_size, PROT_NONE);

    // "Free" the memory by tracking it (we don't actually unmap it here)
    freed_allocations.insert(header);
    allocations.erase(header);
}

void gcheckleaks() {
    for (const auto& alloc : allocations) {
        printf("Memory leak detected: %zu bytes allocated at %s:%d\n",
               alloc->size, alloc->file, alloc->line);
    }
}

static size_t get_aligned_size(AllocHeader* alloc) {
    size_t pagesize = sysconf(_SC_PAGESIZE);
    // Data region size (header + user data) rounded up to page boundary
    size_t data_region_size = (alloc->size + sizeof(AllocHeader) + pagesize - 1) & ~(pagesize - 1);
    return pagesize + data_region_size + pagesize;  // lower guard + data + upper guard
}

void gflushfreed() {
    for (const auto& alloc : freed_allocations) {
        size_t pagesize = sysconf(_SC_PAGESIZE);
        size_t aligned_size = get_aligned_size(alloc);

        // Unmap the memory
        if (munmap((char*)alloc - pagesize, aligned_size) != 0) {
            perror("munmap failed");
        }
    }
    freed_allocations.clear();
}

int ggetnumfreed() {
    return freed_allocations.size();
}

size_t ggetpendingfreesize() {
    size_t total = 0;
    for (const auto& alloc : freed_allocations) {
        total += get_aligned_size(alloc);
    }
    return total;
}