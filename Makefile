# ---- Toolchain (Homebrew LLVM). Ubuntu users: make LLVM=/usr/bin
LLVM    ?= /opt/homebrew/opt/llvm/bin
CC      := $(LLVM)/clang
OBJCOPY := $(LLVM)/llvm-objcopy
QEMU    := qemu-system-riscv32

BUILD := build

# Compile flags: used when turning each .c into a .o
CFLAGS   := -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
            -fno-stack-protector -ffreestanding -I.
# -MMD -MP: also write a .d file listing every header each .c includes
DEPFLAGS := -MMD -MP
# Link flags: only used when linking the .o files together
LDFLAGS  := --target=riscv32-unknown-elf -fuse-ld=lld -nostdlib

KERNEL_SRCS := $(wildcard kernel/*.c) lib/common.c
USER_SRCS   := user/shell.c user/user.c lib/common.c

# kernel/fs.c -> build/kernel-objs/kernel/fs.o, and so on
KERNEL_OBJS := $(KERNEL_SRCS:%.c=$(BUILD)/kernel-objs/%.o)
USER_OBJS   := $(USER_SRCS:%.c=$(BUILD)/user-objs/%.o)

.PHONY: all run clean

all: $(BUILD)/kernel.elf $(BUILD)/disk.tar

run: all
	$(QEMU) -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
	    -drive id=drive0,file=$(BUILD)/disk.tar,format=raw,if=none \
	    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
	    -kernel $(BUILD)/kernel.elf

clean:
	rm -rf $(BUILD)

# ---- Compile: one .c -> one .o
$(BUILD)/kernel-objs/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)/user-objs/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# ---- User program: ELF -> flat binary -> object file the kernel can link in
$(BUILD)/shell.elf: $(USER_OBJS) user/user.ld
	$(CC) $(LDFLAGS) -Wl,-Tuser/user.ld -Wl,-Map=$(BUILD)/shell.map -o $@ $(USER_OBJS)

$(BUILD)/shell.bin: $(BUILD)/shell.elf
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

# Run from inside build/: objcopy names the symbols after the input path,
# so this keeps them as _binary_shell_bin_start/_end/_size
$(BUILD)/shell.bin.o: $(BUILD)/shell.bin
	cd $(BUILD) && $(OBJCOPY) -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# ---- Kernel
$(BUILD)/kernel.elf: $(KERNEL_OBJS) $(BUILD)/shell.bin.o kernel/kernel.ld
	$(CC) $(LDFLAGS) -Wl,-Tkernel/kernel.ld -Wl,-Map=$(BUILD)/kernel.map -o $@ \
	    $(KERNEL_OBJS) $(BUILD)/shell.bin.o

# ---- Disk image
$(BUILD)/disk.tar: $(wildcard disk/*.txt)
	@mkdir -p $(BUILD)
	cd disk && tar cf $(abspath $@) --format=ustar *.txt
	dd if=/dev/zero of=$@ bs=1 count=0 seek=65536 2>/dev/null

# Pull in the generated header dependencies (silently skipped on the first build)
-include $(KERNEL_OBJS:.o=.d) $(USER_OBJS:.o=.d)