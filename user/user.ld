ENTRY(start)

SECTIONS {

    /*
        Base Address
        Different from the Kernel's base address of 0x80200000.
        Application doesn't overlap with the kernel's address space.
    */
    . = 0x1000000;

    /* Machine Code */
    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    /* Read-Only Data */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    /* Read/Write data */
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    /* Read/Write data with an initial value of zero. */
    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);     /* bumps the cursor up to the next multiple of 16 */
        . += 64 * 1024;    /* Advance the current address by 64KB */
        __stack_top = .;   /* stack grows DOWN; toward lower addresses; */

        ASSERT(. < 0x1800000, "too large executable");
    }
}