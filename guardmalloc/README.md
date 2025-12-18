# gmalloc - Guard Page Memory Allocator

A custom memory allocator for debugging buffer overflows, underflows, and use-after-free errors using POSIX memory protection features (mmap/mprotect).

## Features

- **Guard Pages**: Detects buffer overflows and underflows by placing protected memory pages before and after allocations
- **Use-After-Free Detection**: Protects freed memory to catch accidental reads/writes to deallocated regions
- **Leak Reporting**: Tracks allocated memory and reports leaks on program termination
- **High-Level Interface**: Simple C-style allocation with file/line tracking

## Architecture

### Memory Layout

Each allocation has the following layout:

```
[Lower Guard Page (4KB)] [Header | User Data (page-aligned)] [Upper Guard Page (4KB)]
```

- **Lower Guard Page**: Protected with `PROT_NONE`, catches buffer underflows
- **Header**: 24-byte metadata (size, file, line)
- **User Data**: Requested size, followed by padding to page boundary
- **Upper Guard Page**: Protected with `PROT_NONE`, catches buffer overflows

### Page Alignment

The critical insight is that `mprotect()` requires page-aligned addresses. The implementation ensures:

```cpp
size_t data_region_size = (sizeof(AllocHeader) + size + pagesize - 1) & ~(pagesize - 1);
```

This rounds up the combined size of header + user data to the next page boundary, ensuring the upper guard page starts at a page-aligned address.

## Building and Testing

### Build

```bash
make              # Build all test programs
make test         # Build and run test suite
make clean        # Clean build artifacts
```

### Test Suite

The `gmalloc_test` harness runs 4 tests, each demonstrating a memory safety feature:

1. **gmalloc_correctuse**: Tests proper allocation/deallocation cycle
   - Expected: Exit code 0
   - Result: ✓ PASS

2. **gmalloc_useafterfree**: Intentional use-after-free detection
   - Allocates memory, frees it, then attempts to read freed memory
   - Expected: Crash with SIGSEGV or SIGBUS
   - Result: ✓ PASS (Signal 10 - SIGBUS)

3. **gmalloc_bufferoverflow**: Buffer overflow detection
   - Allocates 100 bytes, writes beyond the upper guard page
   - Expected: Crash with SIGSEGV or SIGBUS
   - Result: ✓ PASS (Signal 10 - SIGBUS)

4. **gmalloc_bufferunderflow**: Buffer underflow detection
   - Allocates 5 integers, writes before the lower guard page
   - Expected: Crash with SIGSEGV or SIGBUS
   - Result: ✓ PASS (Signal 10 - SIGBUS)

### Running Tests

```bash
cd /Users/brian/src/GBADoom/guardmalloc
make test
```

Expected output:
```
=== gmalloc Test Harness ===

Test 1: gmalloc_correctuse (should succeed)
  ✓ PASS: gmalloc_correctuse exited successfully

Test 2: gmalloc_useafterfree (should crash with SIGSEGV)
  ✓ PASS: gmalloc_useafterfree crashed with signal 10 as expected

Test 3: gmalloc_bufferoverflow (upper guard page detection)
  ✓ PASS: gmalloc_bufferoverflow crashed with signal 10 as expected

Test 4: gmalloc_bufferunderflow (should crash with SIGSEGV/SIGBUS)
  ✓ PASS: gmalloc_bufferunderflow crashed with signal 10 as expected

=== Test Results ===
Passed: 4/4 tests

✓ All tests passed!
```

## Usage

Replace standard malloc/free with gmalloc/gfree:

```cpp
#include "guardmalloc.h"

int *arr = (int*)GMALLOC(100 * sizeof(int));
// ... use array ...
GFREE(arr);

// Check for leaks before exit
gcheckleaks();
```

The `GMALLOC` and `GFREE` macros automatically track file and line number for better debugging output.

## Implementation Details

### Files

- `guardmalloc.h`: Header with public API
- `guardmalloc.cc`: Implementation of gmalloc, gfree, and leak tracking
- `gmalloc_test.cc`: Test harness using fork/exec/waitpid
- `gmalloc_correctuse.cc`: Correct usage test
- `gmalloc_useafterfree.cc`: Use-after-free detection test
- `gmalloc_bufferoverflow.cc`: Buffer overflow detection test
- `gmalloc_bufferunderflow.cc`: Buffer underflow detection test
- `Makefile`: Build system

### Key Functions

- `gmalloc(size, file, line)`: Allocate memory with guard pages
- `gfree(ptr, file, line)`: Free memory and protect against use-after-free
- `gcheckleaks()`: Report any remaining allocated memory
- `gflushfreed()`: Actually deallocate freed memory

## Platform Notes

- Requires POSIX mmap/mprotect support
- Page size is detected at runtime via `sysconf(_SC_PAGESIZE)`
- Tested on macOS and Linux
- Uses signal SIGBUS (signal 10) for page protection violations (platform-dependent)

## Limitations

- High memory overhead due to guard pages (minimum 2x for small allocations)
- Not thread-safe; use only for single-threaded debugging
- Should not be used in production; intended for development/testing
