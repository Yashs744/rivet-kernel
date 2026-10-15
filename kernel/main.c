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
#include "kernel/sbi.h"
#include "kernel/riscv.h"
#include "kernel/panic.h"
#include "kernel/memory.h"
#include "kernel/trap.h"
#include "kernel/process.h"
#include "kernel/virtio.h"
#include "kernel/fs.h"

// The shell binary embedded into the kernel image (see run.sh)
extern char _binary_shell_bin_start[], _binary_shell_bin_end[], _binary_shell_bin_size[];

void delay(void) {
    for (int i = 0; i < 30000000 ; i++)
        __asm__ __volatile__ ("nop");  // do nothing
}

struct process *process_a;
struct process *process_b;

void process_a_entry(void) {
    printf("Starting Process A\n");

    while (1) {
        putchar('A');
        yield();
    }
}

void process_b_entry(void) {
    printf("Starting Process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void kernel_main(void) {
    // As described the linker script, zero out the whole .bss region
    // The casts to `size_t` are there because subtracting two addresses
    // as plain integers gives the byte count between them.
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);


    /*printf("\n\nHello World %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x80002000);
    printf("Length of Hello,Word is %d\n\n", (int) strlen("Hello World"));*/

    WRITE_CSR(stvec, (uint32_t) kernel_entry);  // tell the CPU where the handler is

    // Initialize the Virtio Blk
    virtio_blk_init();

    // Initialize the Memory
    pool_init_pages();

    fs_init();

    /*char buf[SECTOR_SIZE + 1];
    read_write_disk(buf, 0, 0);
    buf[SECTOR_SIZE] = '\0';
    printf("Read from the disk @sector 0: %s\n", buf);

    strcpy(buf, "Hello World from the kernel!");
    read_write_disk(buf, 0, true);*/

    /*
    // EXCEPTION Example
    __asm__ volatile ("unimp");                 // deliberately cause an exception
    */

    // Paging Example
    /*paddr_t paddr0 = alloc_pages(2);
    paddr_t paddr1 = alloc_pages(1);
    paddr_t paddr2 = alloc_pages(3);

    printf("paddr0 = %x\n", paddr0);
    printf("paddr1 = %x\n", paddr1);
    printf("paddr2 = %x\n\n", paddr2);*/

    /*paddr_t paddr3 = alloc_pages(16384);  // Trigger out of memory*/

    //
    idle_process = create_process(NULL, 0);
    idle_process->pid = 0;
    current_process = idle_process;

    /*process_a = create_process((uint32_t) process_a_entry);
    process_b = create_process((uint32_t) process_b_entry);*/

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