#include "user.h"
#include "lib/syscall_nr.h"

extern char __stack_top[];


int syscall(int sysnum, int arg0, int arg1, int arg2) {

    register int a0 __asm__("a0") = arg0;    // System call argument
    register int a1 __asm__("a1") = arg1;    // System call argument
    register int a2 __asm__("a2") = arg2;    // System call argument
    register int a3 __asm__("a3") = sysnum;  // System call number

    __asm__ volatile(
        "ecall\n"    // Triggers an interrupt to transfer control to the kernel
        : "=r" (a0)  // result in a0
        : "r" (a0), "r" (a1), "r" (a2), "r" (a3)  // input
        : "memory"
    );

    return a0;
}


__attribute__((noreturn))
void exit(void) {
    syscall(SYS_EXIT, 0, 0, 0);
    for (;;);
}

void putchar(char c) {
    syscall(SYS_PUTCHAR, c, 0, 0);
}

int getchar(void) {
    return syscall(SYS_GETCHAR, 0, 0, 0);
}

int readfile(const char *filename, char *buf, int len) {
    return syscall(SYS_READFILE, (int) filename, (int) buf, len);
}

int writefile(const char *filename, const char *buf, int len) {
    return syscall(SYS_WRITEFILE, (int) filename, (int) buf, len);
}

int removefile(const char *filename) {
    return syscall(SYS_REMOVEFILE, (int) filename, 0, 0);
}

int readdir(int index, char *buf, int len) {
    return syscall(SYS_READDIR, index, (int) buf, len);
}

__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    __asm__ volatile(
        "mv sp, %[stack_top]\n"
        "call main\n"
        "call exit\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}