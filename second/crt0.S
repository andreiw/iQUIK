#include <layout.h>

.section ".text"
.globl _start
_start:

        /* From first stage? */
        lis     3,0xdeadbeef@h
        ori     3,3,0xdeadbeef@l
        cmp     0,3,5
        beq     _not_from_of

        /*
         * No, we were booted using bootsector 0.
         * Need to set up basic runtime environment.
         */

        /* Make sure our code is consistent in the I and D caches. */
        lis     14,_start@ha
        addi    3,14,_start@l
        addi    6,14,_end@l
1:      dcbf    0,3
        icbi    0,3
        addi    3,3,0x20
        cmp     0,3,6
        ble     1b
        sync
        isync

/* Use the BAT3 registers to map the 1st 8MB of RAM to 0. */
        mfpvr   9
        rlwinm  9,9,16,16,31            /* r9 = 1 for 601, 4 for 604 */
        cmpi    0,9,1
        bne     4f
        li      7,4                     /* set up BAT registers for 601 */
        li      8,0x7f
        b       5f
4:      li      7,0xff                  /* set up BAT registers for 604 */
        li      8,2
        mtdbatu 3,7
        mtdbatl 3,8
5:      mtibatu 3,7
        mtibatl 3,8
        isync

/* Load up an initial stack pointer */
        lis     1,STACK_TOP@h
        subi    1,1,0x180

_not_from_of:
        b main
