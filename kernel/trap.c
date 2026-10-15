#include "kernel/trap.h"
#include "kernel/riscv.h"
#include "kernel/panic.h"
#include "kernel/syscall.h"

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    /*
     * Exception Handling
     * An exception is the CPU saying: "I can't continue with this instruction on my own.
     * Kernel, please take over." i.e a program asking the kernel for help (a system call)
     * The handler must leave everything exactly as it found it. If it changes even one register
     * by accident, the program resumes with corrupted state.
     */

    __asm__ volatile (
        // Fet the kernel stack of the running process
        "csrrw sp, sscratch, sp\n" // csrrw = CSR read and write; swaps sp and sscratch

        "addi sp, sp, -4*31\n" // ← now sp is free to use: make room for 31 registers (stack grows down)
        // Save ALL general-purpose registers onto the stack
        "sw ra, 4*0(sp)\n"
        "sw gp, 4*1(sp)\n"
        "sw tp, 4*2(sp)\n"
        "sw t0, 4*3(sp)\n"
        "sw t1, 4*4(sp)\n"
        "sw t2, 4*5(sp)\n"
        "sw t3, 4*6(sp)\n"
        "sw t4, 4*7(sp)\n"
        "sw t5, 4*8(sp)\n"
        "sw t6, 4*9(sp)\n"
        "sw a0, 4*10(sp)\n"
        "sw a1, 4*11(sp)\n"
        "sw a2, 4*12(sp)\n"
        "sw a3, 4*13(sp)\n"
        "sw a4, 4*14(sp)\n"
        "sw a5, 4*15(sp)\n"
        "sw a6, 4*16(sp)\n"
        "sw a7, 4*17(sp)\n"
        "sw s0, 4*18(sp)\n"
        "sw s1, 4*19(sp)\n"
        "sw s2, 4*20(sp)\n"
        "sw s3, 4*21(sp)\n"
        "sw s4, 4*22(sp)\n"
        "sw s5, 4*23(sp)\n"
        "sw s6, 4*24(sp)\n"
        "sw s7, 4*25(sp)\n"
        "sw s8, 4*26(sp)\n"
        "sw s9, 4*27(sp)\n"
        "sw s10, 4*28(sp)\n"
        "sw s11, 4*29(sp)\n"

        // Read the saved sp and write it to the stack
        "csrr a0, sscratch\n"  // ← fetch the original sp back
        "sw a0, 4*30(sp)\n"    // a0 is already saved so it's safe to reuse it as a temporary

        // Reset the kernel stack
        "addi a0, sp, 4*31\n"  // a0 = sp + 124 = top of kernel stack again
        "csrw sscratch, a0\n"  // put it back into sscratch

        // Call handle_trap
        "mv a0, sp\n"         // ← pass the pointer in the first argument `a0` register
        "call handle_trap\n"

        // Load ALL general-purpose registers from the stack
        "lw ra, 4*0(sp)\n"
        "lw gp, 4*1(sp)\n"
        "lw tp, 4*2(sp)\n"
        "lw t0, 4*3(sp)\n"
        "lw t1, 4*4(sp)\n"
        "lw t2, 4*5(sp)\n"
        "lw t3, 4*6(sp)\n"
        "lw t4, 4*7(sp)\n"
        "lw t5, 4*8(sp)\n"
        "lw t6, 4*9(sp)\n"
        "lw a0, 4*10(sp)\n"
        "lw a1, 4*11(sp)\n"
        "lw a2, 4*12(sp)\n"
        "lw a3, 4*13(sp)\n"
        "lw a4, 4*14(sp)\n"
        "lw a5, 4*15(sp)\n"
        "lw a6, 4*16(sp)\n"
        "lw a7, 4*17(sp)\n"
        "lw s0, 4*18(sp)\n"
        "lw s1, 4*19(sp)\n"
        "lw s2, 4*20(sp)\n"
        "lw s3, 4*21(sp)\n"
        "lw s4, 4*22(sp)\n"
        "lw s5, 4*23(sp)\n"
        "lw s6, 4*24(sp)\n"
        "lw s7, 4*25(sp)\n"
        "lw s8, 4*26(sp)\n"
        "lw s9, 4*27(sp)\n"
        "lw s10, 4*28(sp)\n"
        "lw s11, 4*29(sp)\n"
        "lw sp, 4*30(sp)\n"

        // Return
        "sret\n"
    );
}

void handle_trap(struct trap_frame *f) {
    /*
     * special registers (CSRs):
     *  sepc    = Program counter at the moment of the exception: the address of the offending instruction.
     *  scause  = Type of exception. The kernel reads this to identify the type of exception.
     *  stval   = Additional information about the exception (e.g., memory address that caused the exception).
     *            Depends on the type of exception.
     *  sstatus = Operation mode (U-Mode/S-Mode) when the exception has occurred.
     */
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        user_pc += 4;
    }
    else {
        printf("the program's a0 was %x\n", f->a0);
        PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }

    WRITE_CSR(sepc, user_pc);
}
