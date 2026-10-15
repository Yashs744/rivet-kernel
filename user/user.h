#pragma once
#include "lib/common.h"

__attribute__((noreturn)) void exit(void);
void putchar(char c);
int getchar(void);
int readfile(const char *filename, char *buf, int len);
int writefile(const char *filename, const char *buf, int len);
int removefile(const char *filename);
int readdir(int index, char *buf, int len);
