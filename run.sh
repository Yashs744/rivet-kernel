#!/bin/bash
set -xue

QEMU=qemu-system-riscv32

OBJCOPY=/opt/homebrew/opt/llvm/bin/llvm-objcopy

# Path to clang and compiler flags
CC=/opt/homebrew/opt/llvm/bin/clang  # Ubuntu users: use CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib -I."

# The specified clang options (CFLAGS variable) are as follows:
# Option	                        Description
# -std=c11	                      Use C11.
# -O2	                            Enable optimizations to generate efficient machine code.
# -g3	                            Generate the maximum amount of debug information.
# -Wall	                          Enable major warnings.
# -Wextra	                        Enable additional warnings.
# --target=riscv32-unknown-elf	  Compile for 32-bit RISC-V.
# -ffreestanding	                Do not use the standard library of the host environment
# -fuse-ld=lld	                  Use LLVM linker (ld.lld).
# -fno-stack-protector	          Disable unnecessary stack protection to avoid unexpected behavior in stack manipulation
# -nostdlib	                      Do not link the standard library.
# -I.	                            Resolve includes like "lib/common.h" and "kernel/fs.h" from the project root.
# -Wl,-Tkernel/kernel.ld	        Specify the linker script.
# -Wl,-Map=build/kernel.map	      Output a map file (linker allocation result).

# All build outputs go into build/
BUILD=build
mkdir -p $BUILD

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser/user.ld -Wl,-Map=$BUILD/shell.map -o $BUILD/shell.elf \
    user/shell.c user/user.c lib/common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary $BUILD/shell.elf $BUILD/shell.bin
# Run objcopy from inside build/: it names the symbols after the input path, so this keeps
# them as _binary_shell_bin_start/_end/_size (not _binary_build_shell_bin_...) for kernel/main.c
(cd $BUILD && $OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o)

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel/kernel.ld -Wl,-Map=$BUILD/kernel.map -o $BUILD/kernel.elf \
    kernel/*.c lib/common.c $BUILD/shell.bin.o

(cd disk && tar cf ../$BUILD/disk.tar --format=ustar *.txt)

# Start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot -global virtio-mmio.force-legacy=false \
    -drive id=drive0,file=$BUILD/disk.tar,format=raw,if=none \
    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
    -kernel $BUILD/kernel.elf