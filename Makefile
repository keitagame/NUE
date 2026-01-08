# ========================================
# Unix-like Kernel Makefile
# ========================================

# ツール定義
CC := gcc
LD := ld
AS := nasm
OBJCOPY := objcopy
GRUB_MKRESCUE := grub-mkrescue

# ターゲットファイル
KERNEL_BIN := kernel.bin
KERNEL_ELF := kernel.elf
ISO_FILE := os.iso

# ソースファイル
C_SOURCES := kernel.c
ASM_SOURCES := 
OBJECTS := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)

# ディレクトリ
ISO_DIR := isodir
BOOT_DIR := $(ISO_DIR)/boot
GRUB_DIR := $(BOOT_DIR)/grub

# コンパイラフラグ
CFLAGS := -m32 \
          -ffreestanding \
          -O2 \
          -Wall \
          -Wextra \
          -fno-pic \
          -fno-pie \
          -fno-stack-protector \
          -fno-builtin \
          -nostdlib \
          -std=gnu99

# リンカフラグ
LDFLAGS := -m elf_i386 \
           -T linker.ld \
           -nostdlib \
           -n

# アセンブラフラグ（NASM使用の場合）
ASFLAGS := -f elf32

# QEMUオプション
QEMU := qemu-system-i386
QEMU_FLAGS := -cdrom $(ISO_FILE) \
              -m 128M \
              -serial stdio

# QEMUデバッグオプション
QEMU_DEBUG_FLAGS := $(QEMU_FLAGS) \
                    -s \
                    -S

# ========================================
# メインターゲット
# ========================================

.PHONY: all
all: $(ISO_FILE)

# ========================================
# カーネルビルド
# ========================================

# ELFファイル作成
$(KERNEL_ELF): $(OBJECTS) linker.ld
	@echo "[LD] Linking $@"
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "[OK] Kernel ELF created: $@"

# バイナリファイル作成（オプション）
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating flat binary"
	$(OBJCOPY) -O binary $< $@
	@echo "[OK] Kernel binary created: $@"

# Cソースファイルのコンパイル
%.o: %.c
	@echo "[CC] Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# アセンブリソースファイルのアセンブル
%.o: %.asm
	@echo "[AS] Assembling $<"
	$(AS) $(ASFLAGS) $< -o $@

# ========================================
# ISO作成
# ========================================

$(ISO_FILE): $(KERNEL_ELF)
	@echo "[GRUB] Creating bootable ISO"
	@mkdir -p $(BOOT_DIR)
	@mkdir -p $(GRUB_DIR)
	@cp $(KERNEL_ELF) $(BOOT_DIR)/kernel.bin
	@echo 'set timeout=0' > $(GRUB_DIR)/grub.cfg
	@echo 'set default=0' >> $(GRUB_DIR)/grub.cfg
	@echo '' >> $(GRUB_DIR)/grub.cfg
	@echo 'menuentry "Unix-like Kernel" {' >> $(GRUB_DIR)/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> $(GRUB_DIR)/grub.cfg
	@echo '    boot' >> $(GRUB_DIR)/grub.cfg
	@echo '}' >> $(GRUB_DIR)/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR) 2>/dev/null
	@echo "[OK] Bootable ISO created: $@"

# ========================================
# 実行とデバッグ
# ========================================

.PHONY: run
run: $(ISO_FILE)
	@echo "[QEMU] Running kernel..."
	$(QEMU) $(QEMU_FLAGS)

.PHONY: debug
debug: $(ISO_FILE)
	@echo "[QEMU] Running kernel in debug mode..."
	@echo "Waiting for GDB connection on localhost:1234"
	$(QEMU) $(QEMU_DEBUG_FLAGS)

.PHONY: gdb
gdb: $(KERNEL_ELF)
	@echo "[GDB] Connecting to QEMU..."
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file $(KERNEL_ELF)" \
	    -ex "break kernel_main" \
	    -ex "continue"

# ========================================
# クリーンアップ
# ========================================

.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts"
	@rm -f $(OBJECTS)
	@rm -f $(KERNEL_ELF) $(KERNEL_BIN)
	@echo "[OK] Build artifacts cleaned"

.PHONY: distclean
distclean: clean
	@echo "[DISTCLEAN] Removing all generated files"
	@rm -f $(ISO_FILE)
	@rm -rf $(ISO_DIR)
	@echo "[OK] All generated files removed"

# ========================================
# 情報表示
# ========================================

.PHONY: info
info:
	@echo "========================================="
	@echo "Kernel Build Information"
	@echo "========================================="
	@echo "Compiler:     $(CC)"
	@echo "Linker:       $(LD)"
	@echo "C Flags:      $(CFLAGS)"
	@echo "LD Flags:     $(LDFLAGS)"
	@echo "Sources:      $(C_SOURCES) $(ASM_SOURCES)"
	@echo "Objects:      $(OBJECTS)"
	@echo "Kernel ELF:   $(KERNEL_ELF)"
	@echo "ISO File:     $(ISO_FILE)"
	@echo "========================================="

.PHONY: size
size: $(KERNEL_ELF)
	@echo "========================================="
	@echo "Kernel Size Information"
	@echo "========================================="
	@size $(KERNEL_ELF)
	@echo "========================================="
	@ls -lh $(KERNEL_ELF) $(ISO_FILE) 2>/dev/null || true

# ========================================
# チェックとテスト
# ========================================

.PHONY: check-tools
check-tools:
	@echo "[CHECK] Verifying required tools..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@which $(LD) > /dev/null || (echo "Error: $(LD) not found" && exit 1)
	@which $(GRUB_MKRESCUE) > /dev/null || (echo "Error: $(GRUB_MKRESCUE) not found" && exit 1)
	@which $(QEMU) > /dev/null || (echo "Error: $(QEMU) not found" && exit 1)
	@echo "[OK] All required tools are available"

.PHONY: test
test: $(ISO_FILE)
	@echo "[TEST] Running kernel test..."
	timeout 5 $(QEMU) $(QEMU_FLAGS) -display none || true
	@echo "[OK] Test completed"

# ========================================
# ヘルプ
# ========================================

.PHONY: help
help:
	@echo "Unix-like Kernel Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build kernel and create ISO (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  run          - Run kernel in QEMU"
	@echo "  debug        - Run kernel in QEMU debug mode"
	@echo "  gdb          - Connect GDB to running QEMU instance"
	@echo "  info         - Display build configuration"
	@echo "  size         - Show kernel size information"
	@echo "  check-tools  - Verify required tools are installed"
	@echo "  test         - Quick test run"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build everything"
	@echo "  make run          # Build and run in QEMU"
	@echo "  make clean all    # Clean build"
	@echo "  make debug        # Run in debug mode, then 'make gdb' in another terminal"

# ========================================
# 依存関係
# ========================================

# 自動依存関係生成
-include $(OBJECTS:.o=.d)

%.d: %.c
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -MF $@
