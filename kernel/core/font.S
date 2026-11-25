/* Embeds the console font file (see font.h). The include path for .incbin is
 * third_party/fonts, set in cmake/kernel.cmake. */

    .section .rodata
    .balign 16
    .global g_font_psf
    .global g_font_psf_end
g_font_psf:
    .incbin "ter-u16n-lat15.psf"
g_font_psf_end:

    .section .note.GNU-stack, "", @progbits
