#include <layout.h>

.section ".start"
.globl _start
.globl _stack_top
.globl _bss_start
.globl _bss_end
_start:

/* Clear BSS. */
        lis     10,_bss_start@h
        ori     10,10,_bss_start@l
        lis     11,_bss_end@h
        ori     11,11,_bss_end@l
        subi    10,10,4
        subi    11,11,4
        li      0, 0
1:      stwu    0,4(10)
        cmp     0,10,11
        bne     1b

/* Load up an initial stack pointer */
        lis     1,_stack_top@h
        ori     1,1,_stack_top@l
        b       iquik_main
