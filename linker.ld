/* linker.ld - カーネル用リンカースクリプト */

ENTRY(_start)

SECTIONS
{
    /* カーネルは1MBからロード */
    . = 0x100000;

    /* マルチブートヘッダーを最初に配置 */
    .multiboot : ALIGN(4K)
    {
        *(.multiboot)
    }

    /* テキストセクション（コード） */
    .text : ALIGN(4K)
    {
        *(.text)
    }

    /* 読み取り専用データ */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* 初期化済みデータ */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* 未初期化データ */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
