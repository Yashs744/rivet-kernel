#pragma once

#include "lib/common.h"

// PROCESS
#define PROCESS_MAX 8        // Maximum number of process

#define PROCESS_UNUSED 0     // Unused process block
#define PROCESS_RUNNABLE 1   // Runnable Process
#define PROCESS_EXITED 2     // Exited Process

// Process Control Block
struct process {
    int state;            // Stores the state of the process; PROCESS_UNUSED or PROCESS_RUNNABLE
    int pid;              // Process ID (unique identifier)
    vaddr_t sp;           // Stack Pointer
    uint32_t *page_table; // Pointer to the L1 page table
    uint8_t stack[8192];  // Kernel Stack; Stores the CPU registers, return address & local vars
};

// Userland Application
#define USER_BASE 0x1000000

extern struct process *current_process;  // Currently running process
extern struct process *idle_process;     // Process to run when there are no runnable processes

struct process *create_process(const void *image, size_t image_size);
void yield(void);
