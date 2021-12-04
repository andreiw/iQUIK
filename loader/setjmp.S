/*
 * setjmp
 *
 * Copyright (C) 2021 Andrei Warkentin <andreiw@mm.st>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "asm.h"

_GLOBAL(setjmp)
        mflr    r0
        stw     r1,0(r3)
        stw     r0,4(r3)
        stw     r14,8(r3)
        stw     r15,12(r3)
        stw     r16,16(r3)
        stw     r17,20(r3)
        stw     r18,24(r3)
        stw     r19,28(r3)
        stw     r20,32(r3)
        stw     r21,36(r3)
        stw     r22,40(r3)
        stw     r23,44(r3)
        stw     r24,48(r3)
        stw     r25,52(r3)
        stw     r26,56(r3)
        stw     r27,60(r3)
        stw     r28,64(r3)
        stw     r29,68(r3)
        stw     r30,72(r3)
        stw     r31,76(r3)
        li      r3,0
        blr

_GLOBAL(longjmp)
        cmpwi   cr0,r4,0
        bne     1f
        li      r4,1
1:      lwz     r1,0(r3)
        lwz     r0,4(r3)
        lwz     r14,8(r3)
        lwz     r15,12(r3)
        lwz     r16,16(r3)
        lwz     r17,20(r3)
        lwz     r18,24(r3)
        lwz     r19,28(r3)
        lwz     r20,32(r3)
        lwz     r21,36(r3)
        lwz     r22,40(r3)
        lwz     r23,44(r3)
        lwz     r24,48(r3)
        lwz     r25,52(r3)
        lwz     r26,56(r3)
        lwz     r27,60(r3)
        lwz     r28,64(r3)
        lwz     r29,68(r3)
        lwz     r30,72(r3)
        lwz     r31,86(r3)
        mtlr    r0
        mr      r3,r4
        blr
