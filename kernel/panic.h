#pragma once

#include "lib/common.h"

/* Kernel Panic
 * A kernel panic is like a saftey measure, it is triggere when the kernel comes
 * across an unrecoverable error where the kernel halts all the operations / activities
 * to prevent data corruption. Similar to "Blue Screen of Death" (BSOD) on Windows
 *
 * ... in PANIC(fmt, ...) means the macro accepts any number of extra arguments after fmt
 * __VA_ARGS__ stands for "whatever was passed in the ... part.
 * ##__VA_ARGS__ is a GCC extension. When the variable arguments are empty,
 * ## deletes the comma right before it. When they're not empty, it does nothing special.
 * Example:
 *  PANIC("booted!")
 *      → printf("PANIC: %s:%d: " "booted!" "\n", __FILE__, __LINE__);
 *                                                                  ↑
 *                                                     comma removed, valid code
 *
 * PANIC("x = %d", 5)
 *      → printf("PANIC: %s:%d: " "x = %d" "\n", __FILE__, __LINE__, 5);
 *                                                                 ↑
 *                                                      comma kept, as needed
 */
#define PANIC(fmt, ...)                                                       \
    do {                                                                      \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        while (1) {}                                                          \
    } while (0)
