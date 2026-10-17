/*
    Firmware (OpenSBI)
           │ jumps to 0x80200000
           ▼
     ┌──────────────┐   sets sp = __stack_top
     │    boot()    │ ────────────────────────┐
     └──────────────┘   jumps (no return)     │
                                              ▼
                                  ┌──────────────────────┐
                                  │    kernel_main()     │
                                  │  1. memset(.bss, 0)  │──► memset()
                                  │  2. loop forever     │
                                  └──────────────────────┘

  ----------------------------------------------------------------------

    RISC-V CPU privilege levels
    ┌─────────────────────────────────────────────┐
    │  U-mode (User)        apps                  │   least privileged
    ├─────────────────────────────────────────────┤
    │  S-mode (Supervisor)  KERNEL                │
    ├─────────────────────────────────────────────┤
    │  M-mode (Machine)     OpenSBI firmware      │   most privileged,
    │                       talks to hardware     │   owns the hardware
    └─────────────────────────────────────────────┘

*/

#include "lib/common.h"
#include "kernel/riscv.h"
#include "kernel/panic.h"
#include "kernel/memory.h"
#include "kernel/trap.h"
#include "kernel/process.h"
#include "kernel/virtio.h"
#include "kernel/fs.h"

// The shell binary embedded into the kernel image (see run.sh)
extern char _binary_shell_bin_start[], _binary_shell_bin_end[], _binary_shell_bin_size[];


void kernel_main(void) {
    // As described the linker script, zero out the whole .bss region
    // The casts to `size_t` are there because subtracting two addresses
    // as plain integers gives the byte count between them.
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    WRITE_CSR(stvec, (uint32_t) kernel_entry);  // tell the CPU where the handler is

    // Initialize the Memory
    pool_init_pages();

    // Initialize the Virtio Blk
    virtio_blk_init();

    // Initialize the file system
    fs_init();

    // Initialize an idle process to go back to.
    idle_process = create_process(NULL, 0);
    idle_process->pid = 0;
    current_process = idle_process;

    printf("\nWelcome to rivet-kernel\n");

    // User Shell
    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

    yield();
    PANIC("Switched to idle process!");

    for (;;) {
        __asm__ __volatile__("wfi");
    }
}

// put the function at the top in .boot section
__attribute__((section(".text.boot")))
// tell compiler to not add its usual setup and cleanup code around the function
__attribute__((naked))
void boot(void) {
    // execution of the kernel starts from the boot function

    // inline assembly code
    // __volatile__ tells the compiler not to optimize this away or move it
    // Copy the address of __stack_top into sp, the stack pointer register
    // Jump to the kernel_main
    __asm__ volatile (
        "mv sp, %[stack_top]\n"
        "j kernel_main"
        :  // OUTPUT ← empty: the asm doesn't hand anything back to C
        : [stack_top] "r" (__stack_top)  // INPUT ← [stack_top] "r" (__stack_top)
        // put the value of __stack_top into some register ("r"), and
        // refer to that register as %[stack_top] in the assembly.
    );
}