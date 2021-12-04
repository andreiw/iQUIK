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

#ifndef ISETJMP_H
#define ISETJMP_H

typedef struct {
   uint32_t r1;  /* 0 */
   uint32_t lr;  /* 4 */
   uint32_t r14; /* 8 */
   uint32_t r15; /* 12 */
   uint32_t r16; /* 16 */
   uint32_t r17; /* 20 */
   uint32_t r18; /* 24 */
   uint32_t r19; /* 28 */
   uint32_t r20; /* 32 */
   uint32_t r21; /* 36 */
   uint32_t r22; /* 40 */
   uint32_t r23; /* 44 */
   uint32_t r24; /* 48 */
   uint32_t r25; /* 52 */
   uint32_t r26; /* 56 */
   uint32_t r27; /* 60 */
   uint32_t r28; /* 64 */
   uint32_t r29; /* 68 */
   uint32_t r30; /* 72 */
   uint32_t r31; /* 76 */
} jmp_buf;

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__ ((__noreturn__));

#endif /* ISETJMP_H */
