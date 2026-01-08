#include <stdint.h>

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static uint16_t cursor = 0;
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = 0x0720; // ' ' (空白), 黒背景・白文字
    }
}
void log_char(char c) {
    VGA[cursor++] = (uint16_t)c | 0x0F00; // 白文字
}

void log_str(const char* s) {
    while (*s) log_char(*s++);
}

__attribute__((section(".multiboot"), used, aligned(4)))
uint32_t multiboot_header[] = {
    0x1BADB002,        // magic
    0x00000003,        // flags (ALIGN | MEMINFO)
    0xE4524FFB         // checksum = -(magic + flags)
};

void kernel_main(void) {
    clear_screen();
    log_str("Hello, keita's World!\n");
    volatile char* vga = (volatile char*)0xB8000;
    
    
    for (;;) __asm__ volatile ("hlt");
}
