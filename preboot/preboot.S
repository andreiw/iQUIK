#include <layout.h>

.section ".start"
.globl _start
.globl _end
.globl _stack_top
_start:

/*
 * iQUIK relies on a few variables being set to be able to boot,
 * because it needs to know where to look for the configuration file.
 * This works fine in an installed system, but for install media, these
 * would need to be typed in and be difficult on the user. In a CHRP
 * environment, these could have been set via a bootinfo script, but on
 * OW machines we have no boot-script support.
 *
 * This is where preboot comes in. It's a simple stub that calls OFW
 * to intepret Forth immediately after itself, and then jumps to
 * iQUIK's expected location.
 *
 * Basically, without preboot, the boot partition is just iquik.b.
 * With preboot, the boot partition contains preboot.b + Forth + iquik.b.
 * The memory map looks like this:
 *
 *
 *  PREBOOT_BASE     |
 *  ...              |
 *  preboot.b _start |
 *  preboot.b _end   | forth script
 *  ...              | 
 *  IQUIK_BASE       | iquik.b _start
 */

/*
 * On entry:
 *           r5 contains the PROM entry.
 *           pc is somewhere between PREBOOT_BASE
 *              and IQUIK_BASE.
 */
	mr	22, 5

/*
 * Ensure consistent I+D caches. We could
 * I suppose fetch the cache line size, but this
 * would explode the code size (since we need to
 * do OF calls) and increase complexity at
 * no real gain.
 *
 * Unfortunately have to make it all crufty and
 * hardcode possible addresses used. Can't get
 * our load-addr with the bcl, because the branch&link
 * might fail due to I+D discrepancy. Yikes...
 *
 * Too bad they got rid of 'clcs' in PowerPC.
 */
	addis	10, 0, PREBOOT_BASE@ha
	addi	10, 10, PREBOOT_BASE@l
	addis 	11, 0, (IQUIK_BASE + IQUIK_SIZE)@ha
	addi	11, 11,(IQUIK_BASE + IQUIK_SIZE)@l
1:	dcbf	0,10
	icbi	0,10
	addi	10,10,4
	cmp	0,10,11
	bne	1b
	sync
	isync

/*
 * Figure out where we are. r23 contains our load-addr.
 */
	bcl	20, 31, offset
offset:	mflr	23

/*
 * Stack.
 */
	addis	1, 23, (_stack_top - offset)@ha
	addi	1, 1, (_stack_top - offset)@l

/*
 * call_prom("interpret", 1, 1, &_end);
 */
	addi	1, 1, -80
	li	4, 1       /* nargs = 1 */
	addis	3, 23, (PROM_INTERPRET - offset)@ha
	addi	3, 3, (PROM_INTERPRET - offset)@l
	stw	3, 0(1)	   /* args.service = "interpret" */
	stw	4, 4(1)    /* args.nargs = nargs */
	li	0, 1       /* nret = 1 */
	stw	0, 8(1)    /* args.nret = nret */
	addis	11, 23, (_end - offset)@ha
	addi	11, 11, (_end - offset)@l
	stw	11, 12(1)  /* args.params[0] = &_end */
	mtlr	22
	mr	3, 1
	blrl
	addi	1, 1, 80
1:	mr	5, 22

/*
 * Could try to strlen the forth buffer, but why, we know
 * QUIK is not position-independent.
 */
	ba	IQUIK_BASE
	
.section ".rodata"
PROM_INTERPRET:	.string "interpret"

