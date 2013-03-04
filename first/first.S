/*
 * First stage bootstrap for Power Macintosh.
 * Because of space restrictions, the original C code
 * has been turned into hand-trimmed assembly
 * (with a little bit of help from gcc :-).
 */
#include <layout.h>

        .file   "first.c"

/*
 * Register usage in main:
 * 14   high part of address
 * 20   bp
 * 21   i
 * 22   prom_entry
 * 25   start
 * 26   fp
 * 27   bdev
 * 28   addr
 * 29   bs
 *
 * Register usage in call_prom:
 * 23   nargs
 * 
 */

        .section        .rodata
.LC0:   .string "finddevice"
.LC1:   .string "/chosen"
.LC1a:  .string "/options"
.LC2:   .string "getprop"
.LC3:   .string "bootpath"
.LC3a:  .string "boot-device"
.LC4:   .string "open"
.LC5:   .string "seek"
.LC6:   .string "read"
.LC7:   .string "exit"
.LC8:   .string "enter"

        .section        ".text"
        .align  2
        .globl  main
        .type   main,@function
        /* main(a1, a2, void (*prom_entry)(struct prom_args *)) */
main:
        lis     14,.LC0@ha
/* Make sure our code is consistent in the I and D caches. */
        addi    3,14,main@l
        li      4,32
        mtctr   4
1:      dcbf    0,3
        icbi    0,3
        addi    3,3,0x20
        bdnz    1b
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

        mr      22,5            /* prom = prom_entry */
        addi    3,14,.LC0@l     /* chosen_handle = call_prom("finddevice, */
        li      4,1             /*                           1, "/chosen"); */
        addi    5,14,.LC1@l
        bl      call_prom

        addi    7,1,8           /* bootpath[0] = 0 */
        li      0,0
        stw     0,0(7)

        mr      5,3
        addi    3,14,.LC2@l     /* call_prom("getprop", 4, chosen_handle, */
        li      4,4             /*           "bootpath", bootpath, */
        addi    6,14,.LC3@l     /*           sizeof(bootpath)); */
        li      8,256
        bl      call_prom

        lbz     0,8(1)          /* if (bootpath[0] == 0) { */
        cmpi    0,0,0
        bne     11f

        addi    3,14,.LC0@l     /*  options_handle = call_prom("finddevice", */
        li      4,1             /*                      1, "/options");  */
        addi    5,14,.LC1a@l
        bl      call_prom
        mr      5,3             /*  call_prom("getprop", 4, options_handle, */
        addi    3,14,.LC2@l     /*            "boot-device", bootpath, */
        li      4,4             /*            sizeof(bootpath)); */
        addi    6,14,.LC3a@l
        li      8,256
        bl      call_prom

11:     addi    3,1,7           /* for (s = bootpath; *s != 0; ++s) */
1:      lbzu    0,1(3)
        cmpwi   0,0,0
        beq     2f
        cmpwi   0,0,58          /* if (*s == ':') { */
        bne     1b
        li      0,0             /* *s = 0; break; } */
        stb     0,0(3)
2:      addi    3,14,.LC4@l     /* bdev = call_prom("open", 1, bootpath); */
        li      4,1
        addi    5,1,8
        bl      call_prom
        mr      27,3
        addi    0,27,1          /* if (bdev == 0 || bdev == -1) */
        cmplwi  0,0,1
        ble     out             /*      goto out; */

        addi    26,14,FIRST_INFO@l      /* fp = FIRST_INFO; */
        lwz     28,16(26)       /* addr = fp->second_base; */
        mr      25,28           /* start = (void *) addr; */
        lwz     29,12(26)       /* bs = fp->blocksize; */
        addi    20,26,0x38-4    /* bp = &fp->blknos[-1]; */
        lwz     21,8(26)        /* i = fp->nblocks; do { */
3:      addi    3,14,.LC5@l     /*      call_prom("seek", 3, bdev, */
        li      4,3             /*                0, *++bp * 512); */
        mr      5,27
        lwzu    31,4(20)
        srwi    6,31,23
        slwi    7,31,9
        bl      call_prom
        mr      16,3
        addi    3,14,.LC6@l     /*      if (call_prom("read", 3, bdev, */
        li      4,3             /*                    addr, bs) != bs) */
        mr      5,27
        mr      6,28
        mr      7,29
        bl      call_prom
        mr      30,3
        cmpw    0,3,29          /*              goto out; */
        bne     out
        add     28,28,29        /*      addr += bs; */
        addic.  21,21,-1        /* } while (--i > 0); */
        bgt     3b

        mr      3,25            /* flush_cache(start, addr); */
4:      dcbf    0,3
        icbi    0,3
        addi    3,3,0x20
        cmplw   3,28
        blt     4b
        sync
        isync

#if 0
        addi    3,14,.LC8@l     /* call_prom("enter", 0); */
        li      4,0
        bl      call_prom
#endif

        mtlr    25              /* (*start)(prom_entry, fp); */
        mr      3,22
        mr      4,26
/* 
 * Let the second stage know that it's being called from the PowerMac
 * first stage bootloader
 *  -- Cort
 */
        lis     5,0xdeadbeef@h
        ori     5,5,0xdeadbeef@l
        blrl
out:
        addi    3,14,.LC8@l     /* call_prom("enter", 0); */
        li      4,0
        bl      call_prom
        addi    3,14,.LC7@l     /* call_prom("exit", 0); */
        li      4,0
        bl      call_prom
        b       out
.Lfe1:
        .size   main,.Lfe1-main

        .align  2
        .globl  call_prom
        .type   call_prom,@function
        .long   0x403c
        /* call_prom(service, nargs, a0, a1, a2, a3) */
call_prom:
        stwu    1,-80(1)
        mflr    0
        stw     0,84(1)
        mr      23,4            /* nargs */
        stw     3,8(1)          /* args.service = service; */
        stw     23,12(1)        /* args.nargs = nargs; */
        li      0,1             /* args.nret = 1; */
        stw     0,16(1)
        stw     5,20(1)         /* args.params[0] = a0; */
        stw     6,24(1)         /* args.params[1] = a1; */
        mtlr    22
        stw     7,28(1)         /* args.params[2] = a2; */
        stw     8,32(1)         /* args.params[3] = a3; */
        addi    3,1,8
        blrl                    /* (*prom)(&args); */
        slwi    23,23,2         /* return args.params[nargs]; */
        add     9,1,23
        lwz     3,20(9)
        lwz     0,84(1)
        mtlr    0
        addi    1,1,80
        blr
.Lfe2:
        .size    call_prom,.Lfe2-call_prom
