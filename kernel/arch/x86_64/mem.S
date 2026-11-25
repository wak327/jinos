/* memcpy, memmove, memset, memcmp for the kernel. The compiler emits calls to
 * these even in freestanding code, so they are written in assembly where it
 * cannot turn them back into calls to themselves. Plain rep-string forms:
 * correct everywhere, fast enough for S0 (ERMS makes them good on modern
 * CPUs). */

    .section .text

    .global memcpy
    .type memcpy, @function
memcpy:
    mov     %rdi, %rax
    mov     %rdx, %rcx
    rep movsb
    ret
    .size memcpy, . - memcpy

    .global memmove
    .type memmove, @function
memmove:
    mov     %rdi, %rax
    mov     %rdx, %rcx
    cmp     %rsi, %rdi
    jbe     1f                      /* dst <= src: forward copy is safe */
    lea     -1(%rsi, %rdx), %rsi    /* overlapping, dst > src: copy backwards */
    lea     -1(%rdi, %rdx), %rdi
    std
    rep movsb
    cld
    ret
1:  rep movsb
    ret
    .size memmove, . - memmove

    .global memset
    .type memset, @function
memset:
    mov     %rdi, %r8
    mov     %esi, %eax
    mov     %rdx, %rcx
    rep stosb
    mov     %r8, %rax
    ret
    .size memset, . - memset

    .global memcmp
    .type memcmp, @function
memcmp:
    xor     %eax, %eax
    test    %rdx, %rdx
    jz      2f
1:  movzbl  (%rdi), %eax
    movzbl  (%rsi), %ecx
    sub     %ecx, %eax
    jnz     2f
    inc     %rdi
    inc     %rsi
    dec     %rdx
    jnz     1b
2:  ret
    .size memcmp, . - memcmp

    .section .note.GNU-stack, "", @progbits
