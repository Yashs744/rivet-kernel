#include "user.h"


void main(void) {
    while (1) {
    prompt:
        printf("rivet-kernel:~$ ");

        char cmdline[128];

        int i = 0;
        for (;;) {
            // Read the character
            char ch = getchar();

            if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            }

            if (ch == 0x7f || ch == '\b') {  // Backspace (DEL or BS)
                if (i > 0) {
                    i--;              // drop last char from buffer
                    printf("\b \b");  // remove it from the screen
                }
            } else if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            }
            else {
                // Write the character
                putchar(ch);
                cmdline[i++] = ch;
            }
        }

        if (cmdline[0] == '\0') {
            continue;
        }

        if (strncmp(cmdline, "echo ", 5) == 0) {
            printf("%s\n", cmdline + 5);
        }
        else if (strcmp(cmdline, "ls") == 0) {
            char dir[128];

            for (int i = 0;; i++) {
                int size = readdir(i, dir, sizeof(dir));

                if (size < 0) break;

                printf("%d Bytes\t%s\n", size, dir);
            }
        }
        else if (strncmp(cmdline, "cat ", 4) == 0) {
            char *filename = cmdline + 4;

            char buf[1024];
            int len = readfile(filename, buf, sizeof(buf)-1);

            if (len < 0) {
                printf("cat: %s: No such file or directory\n", filename);
            }
            else {
                buf[len] = '\0';
                printf("%s", buf);
            }
        }
        else if (strncmp(cmdline, "write ", 6) == 0) {
            char *filename = cmdline + 6;

            // Remove extra spaces before the arguments
            while (*filename == ' ') {
                filename++;
            }

            // Loop till ' ' or END i.e end of filename
            char *p = filename;
            while (*p != '\0' && *p != ' ')
                p++;

            if (*filename == '\0' || *p == '\0') {
                printf("usage: write <filename> <content>\n");
                goto prompt;
            }

            *p = '\0';
            char *content = p+1;

            // Skip extra spaces before the content.
            while (*content == ' ')
                content++;

            if (writefile(filename, content, strlen(content)) < 0)
                printf("write: %s: failed to write\n", filename);
        }
        else if (strncmp(cmdline, "rm ", 3) == 0) {
            char *filename = cmdline + 3;

            if (*filename == '\0' || *filename == ' ') {
                printf("usage: rm <filename>\n");
                goto prompt;
            }

            if (removefile(filename) < 0)
                printf("rm: %s: failed to remove file\n", filename);
        }
        else if (strcmp(cmdline, "exit") == 0 || strcmp(cmdline, "quit") == 0 || strcmp(cmdline, "q") == 0) {
            printf("Bye!\n");
            exit();
        }
        else {
            printf("Unknown command: %s\n", cmdline);
        }
    }
}
