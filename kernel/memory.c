#include "memory.h"
#include "kernel/memory.h"

#include "../lib/common.h"
#include "kernel/panic.h"

// Bump Allocator
paddr_t alloc_pages(uint32_t n) {
    /*
     * static paddr_t next_paddr = (paddr_t) __free_ram; (moved to memory.h)
     * static -> behaves like a global variable i.e value is retained between function calls
     * initial value of next_paddr = __free_ram
     * next_paddr points to the start address of the "next area to be allocated" (free area).
     * When allocating, next_paddr is advanced by the size `n` being allocated.
     */

    if (!next_paddr)
        next_paddr = (paddr_t) __free_ram;

    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    // Check to make sure the allocation is not outside the bound
    if (next_paddr > (paddr_t) __free_ram_end) {
        PANIC("Out of memory!");
    }

    // Fill the region with 0's
    memset((void *) paddr, 0, n * PAGE_SIZE);

    return paddr;
}

// Pool-base Allocation
void pool_init_pages(void) {
    /* Building the list in ascending order. Push from the top down so the head ends up at the LOWEST address
     * Example:
     *  free_list
     *     │
     *     ▼
     *  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
     *  │ next ────────┼────►│ next ────────┼────►│ next = NULL  │
     *  │ (rest unused)│     │ (rest unused)│     │              │
     *  └──────────────┘     └──────────────┘     └──────────────┘
     *     0x802cc000          0x802cd000            0x84246000
     */
    printf("pool_init_pages\n");
    printf("__free_ram_end:%x\t__free_ram:%x\tnext_paddr:%x\n", __free_ram_end, __free_ram, next_paddr);

    for (paddr_t paddr = (paddr_t) __free_ram_end - PAGE_SIZE; paddr > (paddr_t) next_paddr; paddr -= PAGE_SIZE) {
        pool_free_pages(paddr);
    }
}

paddr_t pool_alloc_pages(void) {
    if (!free_list)
        PANIC("Out of memory!");

    struct free_page *page = free_list;  // Get the head pointed by free_list
    free_list = free_list->next;         // pop the head: O(1)

    // Fill with junk
    memset(page, 0, PAGE_SIZE);

    return (paddr_t) page;
}

void pool_free_pages(paddr_t paddr) {
    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("Address is not aligned %x", paddr);

    /*if (paddr >= (paddr_t) __free_ram_end || paddr < (paddr_t) next_paddr)
        PANIC("Out of memory!");*/

    struct free_page *page = (struct free_page *) paddr;

    // Point the next to where free_list is currently point i.e. push onto the head: O(1), LIFO
    page->next = free_list;
    // Update the free_list pointer
    free_list = page;
}

// Page Table Mapping

void map_page(uint32_t *root_table, uint32_t vaddr, paddr_t paddr, uint32_t flags) {

    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("Virtual address is not aligned %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("Physical address is not aligned %x", paddr);

    // Get VPN[1] & VPN[0]
    // value = (x >> position) & mask;
    // & 0x3ff <-- only want 10 bits
    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;

    // Step 1: Make sure the second-level table exists
    if ((root_table[vpn1] & PAGE_V) == 0) {
        // No second-level table exists yet, so allocate one
        // uint32_t new_table = alloc_pages(1);   // Bump Allocator
        uint32_t new_table = pool_alloc_pages();  // Pool-based Allocator

        // new_table / PAGE_SIZE: address → page number
        // << 10: move the page number up to bits 10–31
        // | PAGE_V: fill in the flag bits
        root_table[vpn1] = ((new_table / PAGE_SIZE) << 10) | PAGE_V;
    }

    // Step 2: Find the second-level table
    // Unpacking a PTE back into an address
    // >> 10: drop the flags, leaving only the page number
    // * PAGE_SIZE: page number → address.
    uint32_t *table0 = (uint32_t *) ((root_table[vpn1] >> 10) * PAGE_SIZE);

    // Step 3: Write the final mapping; final Physical Address
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

void map_page_range(uint32_t *root_table, paddr_t paddr_start, paddr_t paddr_end, uint32_t flags) {
    for (paddr_t paddr = paddr_start; paddr < paddr_end; paddr += PAGE_SIZE) {
        map_page(root_table, paddr, paddr, flags);
    }
}