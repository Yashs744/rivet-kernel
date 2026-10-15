#include "kernel/sbi.h"

// general-purpose wrapper
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {

    // GCC extension that means this variable must live in the register named a0
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;  // FID: Function ID, which function within that category
    register long a7 __asm__("a7") = eid;  // EID: Extension ID, which service category

    /*
     * ECALL: Environment call
     * Args: a0 - a5 + FID + EID
     * Ret:
     *  a0: Error code (0 means success)
     *  a1: Return value
     */
    __asm__ volatile (
        "ecall"
        : "=r" (a0), "=r" (a1)
        : "r" (a0), "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
        : "memory"  // clobber: this instruction might read or write memory you can't see.
    );

    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char c) {
    /*
     kernel_main
       │  putchar('H')
       ▼
     putchar ──► sbi_call(72, 0, 0, 0, 0, 0, 0, 1)
                    │
                    │  a0=72, a6=0, a7=1
                    │  ecall
                    ▼
            ┌──────────────────────┐
            │ OpenSBI (M-mode)     │
            │ EID 1 → putchar      │
            │ writes 'H' to UART   │──► "H" appears in the QEMU terminal
            │ returns via mret     │
            └──────────────────────┘
                    │
                    ▼
     sbi_call resumes, returns {error, value}
    */

    // EID 1 is "Console Putchar" from the older legacy SBI extensions.
    sbi_call(c, 0, 0, 0, 0, 0, 0, 1);
}

long getchar(void) {
    // EID 2 is "Console Getchar" from the older legacy SBI extensions.
    struct sbiret result = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);

    return result.error;
}
