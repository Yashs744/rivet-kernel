#include "kernel/syscall.h"

#include "fs.h"
#include "kernel/sbi.h"
#include "kernel/process.h"
#include "kernel/fs.h"
#include "kernel/panic.h"
#include "lib/syscall_nr.h"

/* Handle Syscalls*/
void handle_syscall(struct trap_frame * trap_frame) {

    // Check the value of a3 register to find the type of system call
    switch (trap_frame->a3) {
        case SYS_PUTCHAR: {
            putchar((char) trap_frame->a0);
            break;
        }
        case SYS_GETCHAR: {
            while (1) {
                long ch = getchar();
                if (ch >= 0) {
                    trap_frame->a0 = ch;
                    break;
                }

                yield();
            }

            break;
        }
        case SYS_EXIT: {
            printf("Exiting Process %d\n", current_process->pid);
            current_process->state = PROCESS_EXITED;
            yield();
            PANIC("Unreachable!");
        }
        case SYS_READFILE:
        case SYS_WRITEFILE: {
            const char *filename = (const char *) trap_frame->a0;
            char *buf = (char *) trap_frame->a1;
            int len = trap_frame->a2;

            struct file *file = fs_lookup(filename);

            // Try creating a new file if writing
            if (!file && trap_frame->a3 == SYS_WRITEFILE) {
                file = fs_create(filename);
            }

            if (!file) {
                printf("SYSCALL: file not found: %s\n", filename);
                trap_frame->a0 = -1;
                break;
            }

            if (len > (int) sizeof(file->data))
                len = file->size;

            if (trap_frame->a3 == SYS_WRITEFILE) {
                memcpy(file->data, buf, len);
                file->size = len;
                fs_flush();
            }
            else {
                memcpy(buf, file->data, len);
            }

            trap_frame->a0 = len;
            break;
        }
        case SYS_REMOVEFILE: {
            const char *filename = (const char *) trap_frame->a0;

            struct file *file = fs_lookup(filename);
            if (!file) {
                printf("SYSCALL: file not found: %s\n", filename);
                trap_frame->a0 = -1;
                break;
            }

            // Clear out the block and mark it as not used
            memset(file->data, 0, file->size);
            file->in_use = false;
            file->size = 0;

            // Flush
            fs_flush();

            // Success code
            trap_frame->a0 = 0;
            break;
        }
        case SYS_READDIR: {
            // syscall arguments
            int index = trap_frame->a0;
            char *buf = (char *) trap_frame->a1;
            int len = trap_frame->a2;

            // Return values are put in register a0
            trap_frame->a0 = -1;

            // Read the index-th file
            struct file *file = fs_readdir(index);
            if (!file || file->size <= 0) break;

            int i = 0;
            while (i < len - 1 && file->name[i] != '\0') {
                buf[i] = file->name[i];
                i++;
            }
            buf[i] = '\0';
            trap_frame->a0 = file->size;

            break;
        }
        default: {
            PANIC("unexpected syscall a3=%x\n", trap_frame->a3);
        }
    }
}
