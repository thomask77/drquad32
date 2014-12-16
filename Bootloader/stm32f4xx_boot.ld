/* TODO: Make use of the CCMRAM (for task stacks, etc..) */

MEMORY
{
    /* The first 64k are reserved for the bootloader and parameters */
    BOOT   (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    PARAM0 (rx)  : ORIGIN = 0x08008000, LENGTH = 16K
    PARAM1 (rx)  : ORIGIN = 0x0800C000, LENGTH = 16K
    FLASH  (rx)  : ORIGIN = 0x08010000, LENGTH = 960K

    CCMRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
    SRAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector .isr_vector.*))

        *(.text.Reset_Handler)

        . = ALIGN(4);
    } > BOOT

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        . = ALIGN(4);
    } > FLASH
    __exidx_end = .;

    _etext = .;
    _sidata = .;
    
    .data : AT(_etext)
    {
        _sdata = .;

        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gcc_except_table)
        *(.rodata .rodata*)
        *(.gnu.linkonce.r.*)

        *(vtable vtable.*)
        *(.data .data.*)
        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        _edata = . ;
    } >SRAM

    .bss : 
    {
        _sbss = . ;
        *(.bss .bss.*)
        *(.gnu.linkonce.b*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = . ;
    } >SRAM

    _end = . ;
    PROVIDE (end = .);

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
