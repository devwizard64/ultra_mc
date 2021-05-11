#ifndef _BLOCK_GFX_H_
#define _BLOCK_GFX_H_

#include <ultra64.h>
#include <types.h>

#define BLOCK_MTL_LEN   1

#define BLOCK_NULL              0x00
#define BLOCK_BEDROCK           0x01
#define BLOCK_STONE             0x02
#define BLOCK_COBBLESTONE       0x03
#define BLOCK_DIRT              0x04
#define BLOCK_GRASS             0x05
#define BLOCK_LEN               0x06

struct block_gfx_t
{
    const Gfx *xl;
    const Gfx *xh;
    const Gfx *yl;
    const Gfx *yh;
    const Gfx *zl;
    const Gfx *zh;
    uintptr_t mtl;
};

extern const Vtx vtx_chunk[];
extern const Gfx gfx_block_outline[];
extern const Gfx gfx_block_end_lo[];
extern const Gfx gfx_block_end_hi[];
extern const Gfx *const block_mtl_table[BLOCK_MTL_LEN];
extern const struct block_gfx_t block_gfx_table[BLOCK_LEN];

#endif /* _BLOCK_GFX_H_ */
