# Exception Lifecycle

Example: an invalid instruction `unimp`

```text
 kernel_main
   │
   │  unimp            ← illegal instruction!
   ▼
 ┌──────────────────── CPU does this automatically (hardware) ──────────┐
 │ 1. Checks medeleg: who handles this? → S-mode (OpenSBI set this up)  │
 │ 2. Saves info into special registers (CSRs):                         │
 │       sepc    = address of the unimp instruction                     │
 │       scause  = 2          (why: illegal instruction)                │
 │       stval   = extra detail, depends on the exception type          │
 │       sstatus = which mode we were in (U or S)                       │
 │ 3. Jumps to the address stored in stvec                              │
 └──────────────────────────────────────────────────────────────────────┘
   │
   ▼
 ┌──────────────────── Kernel code (software) ──────────────────────────┐
 │ kernel_entry (assembly)                                              │
 │   4a. Save ALL general-purpose registers onto the stack              │
 │   4b. call handle_trap (pointer to saved registers)                  │
 │                                                                      │
 │ handle_trap (C)                                                      │
 │   reads scause / stval / sepc → decides what to do                   │
 │                                                                      │
 │ kernel_entry (assembly, if handle_trap returns)                      │
 │   5a. Restore ALL registers from the stack                           │
 │   5b. sret → jump back to the address in sepc                        │
 └──────────────────────────────────────────────────────────────────────┘
```