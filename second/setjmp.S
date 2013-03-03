	.globl	__sigsetjmp
__sigsetjmp:
	mflr	0
	stw	1,0(3)
	stw	2,4(3)
	stw	0,8(3)
	stw	14,12(3)
	stw	15,16(3)
	stw	16,20(3)
	stw	17,24(3)
	stw	18,28(3)
	stw	19,32(3)
	stw	20,36(3)
	stw	21,40(3)
	stw	22,44(3)
	stw	23,48(3)
	stw	24,52(3)
	stw	25,56(3)
	stw	26,60(3)
	stw	27,64(3)
	stw	28,68(3)
	stw	29,72(3)
	stw	30,76(3)
	stw	31,80(3)
	li	3,0
	blr

	.globl	longjmp
longjmp:
	cmpwi	0,4,0
	bne	1f
	li	4,1
1:	lwz	1,0(3)
	lwz	2,4(3)
	lwz	0,8(3)
	lwz	14,12(3)
	lwz	15,16(3)
	lwz	16,20(3)
	lwz	17,24(3)
	lwz	18,28(3)
	lwz	19,32(3)
	lwz	20,36(3)
	lwz	21,40(3)
	lwz	22,44(3)
	lwz	23,48(3)
	lwz	24,52(3)
	lwz	25,56(3)
	lwz	26,60(3)
	lwz	27,64(3)
	lwz	28,68(3)
	lwz	29,72(3)
	lwz	30,76(3)
	lwz	31,80(3)
	mtlr	0
	mr	3,4
	blr
