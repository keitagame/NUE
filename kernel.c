// kernel.c - シンプルなUnix互換カーネル
// GRUBマルチブート対応

// 基本型定義（freestanding環境用）
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;

#define NULL ((void*)0)

// マルチブートヘッダー定義
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS 0x00000003
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

// VGAテキストモード設定
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// 色定義
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

// マルチブート構造体
struct multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} __attribute__((packed));

// グローバル変数
static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color;

// マルチブートヘッダー（.multibootセクション）
__attribute__((section(".multiboot")))
struct multiboot_header multiboot = {
    MULTIBOOT_HEADER_MAGIC,
    MULTIBOOT_HEADER_FLAGS,
    MULTIBOOT_CHECKSUM
};

// ユーティリティ関数
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// ターミナル初期化
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_scroll();
        }
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_scroll();
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// I/Oポート操作
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 簡易メモリ管理
#define HEAP_START 0x100000
#define HEAP_SIZE 0x100000
static uint8_t* heap_current = (uint8_t*)HEAP_START;

void* kmalloc(size_t size) {
    void* ptr = heap_current;
    heap_current += size;
    if (heap_current > (uint8_t*)(HEAP_START + HEAP_SIZE))
        return NULL;
    return ptr;
}

// プロセス構造体（簡易版）
typedef struct {
    int pid;
    int state;
    char name[32];
} process_t;

#define MAX_PROCESSES 64
static process_t processes[MAX_PROCESSES];
static int next_pid = 1;

// プロセス管理
int create_process(const char* name) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == 0) {
            processes[i].pid = next_pid++;
            processes[i].state = 1;
            
            size_t j = 0;
            while (name[j] && j < 31) {
                processes[i].name[j] = name[j];
                j++;
            }
            processes[i].name[j] = '\0';
            
            return processes[i].pid;
        }
    }
    return -1;
}

// カーネルメインエントリーポイント
void kernel_main(void) {
    terminal_initialize();
    
    // ウェルカムメッセージ
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("========================================\n");
    terminal_writestring("  Unix-like Kernel v0.1\n");
    terminal_writestring("========================================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("[OK] Kernel loaded\n");
    terminal_writestring("[OK] VGA text mode initialized\n");
    terminal_writestring("[OK] Memory manager initialized\n");
    
    // プロセステーブル初期化
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = 0;
    }
    terminal_writestring("[OK] Process table initialized\n");
    
    // テストプロセス作成
    int pid1 = create_process("init");
    int pid2 = create_process("kernel_daemon");
    
    terminal_writestring("[OK] Process 'init' created (PID: ");
    terminal_putchar('0' + pid1);
    terminal_writestring(")\n");
    
    terminal_writestring("[OK] Process 'kernel_daemon' created (PID: ");
    terminal_putchar('0' + pid2);
    terminal_writestring(")\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("\nKernel is running!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    terminal_writestring("System ready. Halting CPU...\n");
    
    // CPUを停止
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// エントリーポイント（アセンブリから呼ばれる）
void _start(void) {
    kernel_main();
}
