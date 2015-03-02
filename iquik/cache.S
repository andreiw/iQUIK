#include "asm.h"

/*
 * Flush the dcache and invalidate the icache for a range of addresses.
 *
 * flush_cache(addr, len)
 */

_GLOBAL(flush_cache)
        addi    4,4,0x1f        /* len = (len + 0x1f) / 0x20 */
        rlwinm. 4,4,27,5,31
        mtctr   4
        beqlr
1:      dcbf    0,3
        icbi    0,3
        addi    3,3,0x20
        bdnz    1b
        sync
        isync
        blr
