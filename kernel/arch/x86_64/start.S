/* Kernel entry. Limine enters here in 64-bit mode with paging on, interrupts
 * off, and a bootloader-provided stack (see docs/memory-layout.md). All we do
 * is terminate the frame-pointer chain and hand over to C++. */

    .section .text
    .global _start
    .type _start, @function
_start:
    xor     %rbp, %rbp
    and     $-16, %rsp
    call    kmain
    /* kmain never returns; if it does, stop quietly. */
    cli
1:  hlt
    jmp     1b
    .size _start, . - _start

    .section .note.GNU-stack, "", @progbits
