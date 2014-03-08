#include <layout.h>
#include "asm.h"

.section ".start"
.globl _end
.globl _stack_top
.globl _bss_start
.globl _bss_end

_GLOBAL(_start)

/*
 * Ensure consistent I+D caches. We could
 * I suppose fetch the cache line size, but this
 * would explode the code size (since we need to
 * do OF calls) and increase complexity at
 * no real gain.
 *
 * Too bad they got rid of 'clcs' in PowerPC.
 */
	lis	10,_start@h
	ori	10,10,_start@l
	lis	11,_end@h
	ori	11,11,_end@l
1:	dcbf	0,10
	icbi	0,10
	addi	10,10,4
	cmp	0,10,11
	bne	1b
	sync
	isync

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

/*
 * Load up an initial stack pointer. So
 * the stack doesn't ruin the preboot_script
 * area, skip the linkage area, while maintaining
 * 16-byte alignment.
 */
        lis     1,_stack_top@h
        ori     1,1,_stack_top@l
        addi    1,1,-16
        b       iquik_main

.section ".preboot"
.global _preboot_script
_preboot_script:
        .p2align 4
	.long   PREBOOT_MAGIC

