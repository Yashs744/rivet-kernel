#include "kernel/process.h"

#include "memory.h"
#include "kernel/memory.h"
#include "kernel/riscv.h"
#include "kernel/virtio.h"
#include "kernel/panic.h"

__attribute__((naked))
void user_entry(void) {
    /*
     * user_entry (S-mode)
     *     │ sepc = 0x1000000 (USER_BASE), SPP = 0
     *     │ sret
     *     ▼
     * shell's first instruction (U-mode)
     *
     * The switch from S-Mode to U-Mode is done with the `sret` instruction when SPP = 0
     */
    __asm__ __volatile__(
        "csrw sepc, %[sepc]\n"
        "csrw sstatus, %[sstatus]\n"
        "sret\n"
        :
        : [sepc] "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE | SSTATUS_SUM)
    );
}

__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp) {
    /*
     * 1. Push ra and s0–s11 onto the CURRENT stack     (save "where I was")
     * 2. Store sp into prev->sp                        (remember my stack)
     * 3. Load sp from next->sp                         (switch to the other stack)
     * 4. Pop ra and s0–s11 from the NEW stack          (restore "where they were")
     * 5. ret  →  jump to ra                            (continue that process)
     */
    __asm__ volatile (
        // Save the callee-saved registers on the the current process's stack
        "addi sp, sp, -13 * 4\n" // Allocate stack space
        "sw ra, 4*0(sp)\n"
        "sw s0, 4*1(sp)\n"
        "sw s1, 4*2(sp)\n"
        "sw s2, 4*3(sp)\n"
        "sw s3, 4*4(sp)\n"
        "sw s4, 4*5(sp)\n"
        "sw s5, 4*6(sp)\n"
        "sw s6, 4*7(sp)\n"
        "sw s7, 4*8(sp)\n"
        "sw s8, 4*9(sp)\n"
        "sw s9, 4*10(sp)\n"
        "sw s10, 4*11(sp)\n"
        "sw s11, 4*12(sp)\n"

        // Switch the stack pointer
        "sw sp, (a0)\n"         // push the stack pointer of prev process to a0
        "lw sp, (a1)\n"         // load the new stack pointer of next process from a1

        // Restore callee-saved registers from the next process's stack
        "lw ra, 0*4(sp)\n"
        "lw s0, 4*1(sp)\n"
        "lw s1, 4*2(sp)\n"
        "lw s2, 4*3(sp)\n"
        "lw s3, 4*4(sp)\n"
        "lw s4, 4*5(sp)\n"
        "lw s5, 4*6(sp)\n"
        "lw s6, 4*7(sp)\n"
        "lw s7, 4*8(sp)\n"
        "lw s8, 4*9(sp)\n"
        "lw s9, 4*10(sp)\n"
        "lw s10, 4*11(sp)\n"
        "lw s11, 4*12(sp)\n"

        // Update the stack pointer
        "addi sp, sp, 13 * 4\n"

        // Return
        "ret\n"
    );
}

// Process Initialization Function
struct process procs[PROCESS_MAX];

// struct process *create_process(uint32_t pc) {
struct process *create_process(const void *image, size_t image_size) {
    struct process *proc = NULL;

    // Iterate over all the process and find an unused process block
    int i;
    for (i = 0; i < PROCESS_MAX; i++) {
        if (procs[i].state == PROCESS_UNUSED) {
            proc = &procs[i];  // point the proc to the unused block
            break;
        }
    }

    if (!proc) {
        PANIC("No free process slots available!");
    }

    /*
     * Faking the stack of a "paused" process
     * A brand-new process has never run, so it never called switch_context
     * and has nothing saved.But switch_context expects those 13 words to be there.
     * SOLUTION: write them by hand. Build the exact stack layout a paused process would have,
     * with one trick: put the process's starting address into the ra slot.
     *
     *  proc->stack (8 KB)
     *  ┌─────────────────────────────┐ ← &stack[8192]  (starting sp)
     *  │ s11 = 0                     │   1st push
     *  │ s10 = 0                     │   2nd push
     *  │ s9  = 0                     │
     *  │ ...                         │
     *  │ s0  = 0                     │
     *  │ ra  = pc  ◄─────────────────┼── 13th push: the starting address!
     *  ├─────────────────────────────┤ ← final sp, saved into proc->sp
     *  │  (empty: room for the       │
     *  │   process's function calls) │
     *  └─────────────────────────────┘ ← &stack[0]
     */
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    // *--sp = value means "first move sp down by one word (4 bytes), then write value there."
    *--sp = 0;              // s11
    *--sp = 0;              // s10
    *--sp = 0;              // s9
    *--sp = 0;              // s8
    *--sp = 0;              // s7
    *--sp = 0;              // s6
    *--sp = 0;              // s5
    *--sp = 0;              // s4
    *--sp = 0;              // s3
    *--sp = 0;              // s2
    *--sp = 0;              // s1
    *--sp = 0;              // s0
    // *--sp = (uint32_t) pc;  // ra
    *--sp = (uint32_t) user_entry;

    // Map the kernel pages
    // Each process gets its own root table, which is what gives each process its own address space.

    // Allocate one zeroed page to be this process's root table
    uint32_t *page_table = (uint32_t *) pool_alloc_pages();

    // Visit every page from the start of the kernel to the end of free RAM, stepping 4 KB at a time
    /*for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        // Map each page with virtual address = physical address.
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);*/

    /*
     * Improvement
     * code should be read + execute only, constants read-only, and everything else read + write.
     *  .text      R   X     can read & run it, can't modify it
     *  .rodata    R         can only read it
     *  .data      R W       can read and modify, can't run it
     *  .bss       R W
     *  stack      R W
     *  free RAM   R W
     */
    map_page_range(page_table, (paddr_t) __kernel_base, (paddr_t) __rodata, PAGE_R | PAGE_X);
    map_page_range(page_table, (paddr_t) __rodata, (paddr_t) __data, PAGE_R);
    map_page_range(page_table, (paddr_t) __data, (paddr_t) __free_ram_end, PAGE_R | PAGE_W);

    // Mapping the register region for Virtio
    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

    // Map the user pages
    for (uint32_t offset = 0; offset < image_size; offset += PAGE_SIZE) {
        // Allocate the page i.e get some free memory
        // paddr_t page = alloc_pages(1);
        paddr_t page = pool_alloc_pages();

        // Data to copy is smaller than the page size
        size_t remaining = image_size - offset;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        // Copy the content of the image / program (machine code and data) into the free memory (page of RAM)
        memcpy((void *) page, image + offset, copy_size);

        // Write a page table entry (PTE) - says where that page is, plus permission flags.
        map_page(page_table, USER_BASE + offset, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }

    proc->pid = i + 1;
    proc->state = PROCESS_RUNNABLE;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;

    return proc;
}

// Scheduler (yield)
struct process *current_process;  // Currently running process
struct process *idle_process;     // Idle Process; process to run when there are no runnable processes

void yield(void) {
    struct process *next = idle_process;

    // Search for a runnable process
    for (int i = 0; i < PROCESS_MAX; i++) {
        struct process *proc = &procs[(current_process->pid + i) % PROCESS_MAX];

        if (proc->state == PROCESS_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // next == current_process i.e no other runnable process is available except the current one
    if (next == current_process)
        return;

    // Write the top of next's kernel stack into `sscratch`
    // &stack[sizeof(stack)] = &stack[8192] ◄── sscratch points HERE
    // As each process has it's own kernel stack, the sscratch needs to be
    // update before switching the context.
    __asm__ __volatile__ (
        // Switch the process's page table when context switching
        "sfence.vma\n"
        "csrw satp, %[satp]\n"  // switch page tables by specifying the first-level page table in satp
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
          [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    // Make the context switch
    struct process *previous_process = current_process;
    current_process = next;
    switch_context(&previous_process->sp, &next->sp);
}
