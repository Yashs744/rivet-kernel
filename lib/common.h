#pragma once

/*
Kernel does not have any standard library like <stdlib.h>
or <stdio.h> as these come from libc which assumes you have an OS.
Kernel defines the basic types itself.
*/
typedef int bool;
typedef unsigned char uint8_t;  // 1 Bytes
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;  // 8 Bytes
typedef unsigned long long uint64_t;

typedef uint32_t size_t;    // sizes & lengths; 32-bit RISC-V; Addresses are 32 bits wide
typedef uint32_t paddr_t;   // Physical Address
typedef uint32_t vaddr_t;   // Virtual Address

#define true 1
#define false 0
#define NULL ((void *) 0)
#define PAGE_SIZE 4096

/*
 * Rounds value up to the nearest multiple of align. If it's already a multiple, it's left alone.
 * align_up(0,    4096) == 0
 * align_up(1,    4096) == 4096
 * align_up(4096, 4096) == 4096     // already aligned, no change
 * align_up(4097, 4096) == 8192
 */
#define align_up(value, align) __builtin_align_up(value, align)

// The predicate form — returns true if value is already a multiple of align:
#define is_aligned(value, align) __builtin_is_aligned(value, align)

/*
 * Gives the byte offset of a field from the start of its struct:
 * Example
 *  struct process {
 *      int pid;             // offsetof(struct process, pid)   == 0
 *      int state;           // offsetof(struct process, state)  == 4
 *      vaddr_t sp;          // offsetof(struct process, sp)     == 8
 *  };
 */
#define offsetof(type, member) __builtin_offsetof(type, member)

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void *memset(void *buf, char val, size_t size);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void printf(const char *fmt, ...);