# rivet-kernel

A small 32-bit RISC-V hobby kernel, written from scratch in C with a bit of assembly, to explore how operating systems work.
It runs in S-mode on QEMU's `virt` machine, on top of the OpenSBI firmware.

## Features & Roadmap

Work in progress. Checked items work today; unchecked items are planned.

- [x] **Traps & system calls**: one trap entry point that saves and restores every register, and calls from user mode via `ecall`
- [x] **Memory**: a simple bump allocator.
  - [x] Pool-based (free-list) allocator for 4 KB pages
  - [ ] Free a process's pages when it exits
- [x] **Paging**: Sv32 two-level page tables, one address space per process
- [x] **Processes**: up to 8 processes with cooperative scheduling (`yield`) and context switching
  - [ ] Preemptive scheduling with timer interrupts
- [x] **User mode**: the shell runs in `U-mode` and is embedded in the kernel image
  - [ ] Load user programs from the disk instead of embedding them
- [x] **Disk**: ~`virtio-blk` driver (currently legacy MMIO interface)~
  - [x] `virtio-blk` driver modern (virtio 1.0) MMIO interface
- [x] **File system**: files stored in a tar archive on the disk, which can be read, written, listed and removed
- [x] **Shell** commands: `echo`, `ls`, `cat`, `write`, `rm`, `exit`

## Requirements

- LLVM / Clang with the RISC-V target, plus `lld` and `llvm-objcopy`
- `qemu-system-riscv32`

On macOS:

```sh
brew install llvm lld qemu
```

The Makefile expects LLVM at `/opt/homebrew/opt/llvm/bin`. Elsewhere, pass the path yourself, for example `make LLVM=/usr/bin`.

## Build & run

```sh
make run     # build the kernel and disk image, then boot in QEMU
make clean   # remove build/
```

Quit QEMU with `Ctrl-A` then `X`. Files in `disk/` are packed into `build/disk.tar` and attached as the virtio disk.

```
$ ls
12 Bytes        hello.txt
13 Bytes        meow.txt
$ write notes.txt hello from rivet
$ ls 
12 Bytes        hello.txt
13 Bytes        meow.txt
16 Bytes        notes.txt
$ cat notes.txt
hello from rivet
$ exit
Bye!
Exiting Process 2
```

## Project structure

```
rivet-kernel/
├── Makefile            # build kernel + shell, pack the disk image, run QEMU
├── kernel/
│   ├── kernel.ld       # kernel memory layout: .text, .rodata, .data, .bss, stack, free RAM
│   ├── main.c          # boot entry point and kernel_main
│   ├── trap.c/.h       # trap entry (kernel_entry) and handle_trap
│   ├── syscall.c/.h    # system call handlers
│   ├── process.c/.h    # process table, create_process, yield, context switch
│   ├── memory.c/.h     # page allocators and Sv32 page-table mapping
│   ├── virtio.c/.h     # virtio-blk driver
│   ├── fs.c/.h         # tar-based file system
│   ├── sbi.c/.h        # OpenSBI calls (console I/O)
│   ├── riscv.h         # CSR read/write helpers and constants
│   └── panic.h         # PANIC macro
├── lib/
│   ├── common.c/.h     # shared helpers used by kernel and user code: printf, memset, memcpy, strcmp, …
│   └── syscall_nr.h    # system call numbers
├── user/
│   ├── user.ld         # user program layout (loaded at 0x1000000)
│   ├── user.c/.h       # user-space runtime and system call wrappers
│   └── shell.c         # the shell
├── disk/               # files packed into the disk image
└── docs/               # notes: exceptions, register usage, memory, page tables
```

## References

- [Operating System in 1,000 Lines](https://operating-system-in-1000-lines.vercel.app/en/)
- [Memory Allocation Strategies](https://www.gingerbill.org/series/memory-allocation-strategies/)
- [The Adventures of OS: Making a RISC-V Operating System using Rust](https://osblog.stephenmarz.com/)
- [Writing an OS in Rust](https://os.phil-opp.com/): a good resource for understanding exceptions, faults and paging
- [Virtio specification](https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html)
- [RISC-V Supervisor Binary Interface Specification](https://www.scs.stanford.edu/~zyedidia/docs/riscv/riscv-sbi.pdf)
- [Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)

## License

[MIT](LICENSE)
