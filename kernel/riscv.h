#pragma once

#include "lib/common.h"

/*
 * #reg is the preprocessor's "stringify" operator. READ_CSR(scause) turns #reg into "scause",
 * and C glues adjacent strings together, so the assembly becomes "csrr %0, scause".
 *
 * ({ ... }) is a GCC/Clang extension called a statement expression. It lets a block of
 * statements produce a value: the last line, __tmp;, is the result.
 * That's what makes uint32_t x = READ_CSR(scause); work.
 */
#define READ_CSR(reg)                                                         \
    ({                                                                        \
        unsigned long __tmp;                                                  \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                 \
        __tmp;                                                                \
    })

// No need for ({ ... }) for  WRITE_CSR as it does not produce a value (result)
#define WRITE_CSR(reg, value)                                                 \
    do {                                                                      \
        uint32_t __tmp = (value);                                             \
        __asm__ volatile("csrw " #reg ", %0" : : "r"(__tmp));                 \
    } while (0)

// Page Table: switching on Sv32 paging in `satp`
#define SATP_SV32 (1u << 31)  // 1 bit in the `satp` register, indicates "enable paging in Sv32 mode"

// Userland Application
#define SSTATUS_SPIE (1 << 5)  // U-Mode (User Mode)

// System Call Handlers
#define SCAUSE_ECALL 8
#define SSTATUS_SUM  (1 << 18)
