ENTRY(_start)

SECTIONS {
  . = 1M;

  .text : {
    KEEP(*(.multiboot))   /* ← これが最重要 */
    *(.text*)
  }

  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }
}
