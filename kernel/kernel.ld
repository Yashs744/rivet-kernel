/*
     Address                Contents
     ─────────────────────────────────────────────────────────────
     0x80000000  ┌───────────────────────────────┐  ← start of RAM
                 │   OpenSBI firmware (2 MB)     │  ← not part of our kernel
                 │                               │
  __kernel_base →├───────────────────────────────┤  ← 0x80200000: kernel starts here
                 │ .text.boot   (boot function)  │  ← ENTRY: CPU jumps here first
                 │ .text        (all other code) │
                 ├───────────────────────────────┤  (aligned to 4 bytes)
                 │ .rodata      (constants,      │
                 │               string literals)│
                 ├───────────────────────────────┤  (aligned to 4 bytes)
                 │ .data        (initialized     │
                 │               global vars)    │
        __bss  → ├───────────────────────────────┤  (aligned to 4 bytes)
                 │ .bss         (zero-initialized│
                 │               global vars)    │
     __bss_end → ├───────────────────────────────┤
                 │                               │
                 │   STACK (128 KB)              │   ▲ stack grows DOWN
                 │                               │   │ toward lower addresses
     __stack_top→├───────────────────────────────┤   │ starts here
                 │ (padding, possibly 0 bytes)   │  ← ALIGN(4096) skips ahead to
                 │                               │    the next 4 KB boundary
     __free_ram →├───────────────────────────────┤  (aligned to 4096 bytes)
                 │                               │
                 │   FREE RAM (64 MB)            │  ← handed out page by page
                 │   page 0      (4 KB)          │    by alloc_pages()
                 │   page 1      (4 KB)          │
                 │   ...                         │
                 │   page 16383  (4 KB)          │    64 MB / 4 KB = 16384 pages
                 │                               │
 __free_ram_end →└───────────────────────────────┘

     Kernel's address space = __kernel_base ... __free_ram_end
     (everything the kernel owns: code, data, stack, and free RAM)
*/

ENTRY(boot)  /* Kernel starts here */

SECTIONS {
    /*
        Base Address
        On RISC-V QEMU, RAM starts at 0x80000000, and the firmware (OpenSBI) occupies the first 2 MB.
        Kernel gets placed right after it
    */
    . = 0x80200000;

    /*
        Kernel's address space
    */
    __kernel_base = .;

    /*
        .text.boot is always at the beginning ← ENTRY: CPU jumps here first
        KEEP(...) stops the linker from deleting it during "garbage collection" of unused code.
        *(.text.boot): pulls in all remaining code from every file e.g. .text.main or .text.printf
    */
    .text :{
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    /* Read-Only Data */
    . = ALIGN(4096);
    __rodata = .;
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    /* Read/Write data */
    . = ALIGN(4096);
    __data = .;
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    /*
        Read/Write data with an initial value of zero.
        __bss = .; records the cursor's address
        __bss_end = .; records it at the end
    */
    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);     /* bumps the cursor up to the next multiple of 4 */
    . += 128 * 1024;  /* Advance the current address by 128KB */
    __stack_top = .;  /* stack grows DOWN; toward lower addresses; */

    /* Memory Allocation*/
    . = ALIGN(4096);        /* Keep the memory region aligned to a 4KB boundary */
    __free_ram = .;         /* Starting Address of the region */
    . += 64 * 1024 * 1024;  /* 64MB */
    __free_ram_end = .;     /* End Address of the region */

    /* Labels like __SYMBOL_NAME can be refered like `extern char __SYMBOL_NAME` in C */
}
