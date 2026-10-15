#pragma once

#include "lib/common.h"

// Page Table
// Check `page-table.md` for details
#define PAGE_V    (1 << 0)    // "Valid" bit
#define PAGE_R    (1 << 1)    // Readable
#define PAGE_W    (1 << 2)    // Writable
#define PAGE_X    (1 << 3)    // Executable
#define PAGE_U    (1 << 4)    // User; may user-mode code access it?

// Labels defined in the linker script are accessed via `extern char symbol_name`
extern char __rodata[], __data[], __bss[], __bss_end[], __stack_top[], __free_ram[], __free_ram_end[], __kernel_base[];
static paddr_t next_paddr;

paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *root_table, uint32_t vaddr, paddr_t paddr, uint32_t flags);
void map_page_range(uint32_t *root_table, paddr_t paddr_start, paddr_t paddr_end, uint32_t flags);

/*
 * Pool-Based Allocation
 * A pool allocator takes the memory and divides it into chunks of fixed-size (one 4 KB page) blocks.
 * A free list (https://en.wikipedia.org/wiki/Free_list) is used to keep track of the blocks which
 * internally stores a linked-list of the free blocks.
 */
struct free_page {           // A free memory page that can be reused.
    struct free_page *next;  // Stores the address of the next free page.
};

// free_list: stores the address of the first free page. initially, free_list --> NULL
// `*` means it is a pointer variable that can hold the address of a `struct free_page`
static struct free_page *free_list;

void pool_init_pages(void);
void pool_free_pages(paddr_t paddr);
paddr_t pool_alloc_pages(void);
