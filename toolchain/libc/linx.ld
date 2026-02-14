/*
 * linx-libc: Linker script for LinxISA
 * 
 * This linker script defines the memory layout for programs
 * running on LinxISA systems.
 */

/* Entry point */
ENTRY(_start)

/* Stack size */
STACK_SIZE = 0x10000;

/* Memory regions */
MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 256K
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
}

/* Sections */
SECTIONS {
    /* .text section - code */
    .text : {
        *(.text*)
        *(.rodata*)
    } > RAM
    
    /* .data section - initialized data */
    .data : {
        *(.data*)
    } > RAM
    
    /* .bss section - uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM
    
    /* Stack */
    . = ALIGN(16);
    _estack = ORIGIN(RAM) + LENGTH(RAM);
    
    /* End of program */
    _end = .;
}
