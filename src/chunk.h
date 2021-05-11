#ifndef _CHUNK_H_
#define _CHUNK_H_

#include <ultra64.h>
#include <types.h>

#include "world.h"
#include "block_gfx.h"

#define BLOCK_S         128
#define BLOCK_W         BLOCK_S
#define BLOCK_H         BLOCK_S
#define BLOCK_D         BLOCK_S

#define CHUNK_S         16
#define CHUNK_W         CHUNK_S
#define CHUNK_H         CHUNK_S
#define CHUNK_D         CHUNK_S
#define CHUNK_LEN       (CHUNK_W*CHUNK_H*CHUNK_D)

#define CHUNK_FLAG_INIT_GFX     0x0001

#define block_xyz(x, y, z) block[CHUNK_W*(CHUNK_D*(y)+(z))+(x)]

struct chunk_t
{
    struct chunk_t *xl;
    struct chunk_t *xh;
    struct chunk_t *yl;
    struct chunk_t *yh;
    struct chunk_t *zl;
    struct chunk_t *zh;
    u16     flag;
    s16     pos[3];
    u8     *block;
    Gfx    *gfx[BLOCK_MTL_LEN];
};

extern struct chunk_t *chunk_seek(struct chunk_t *, s32, s32, s32);
extern void chunk_block_write(struct chunk_t *, u32, u32, u32, u32);
extern void chunk_free_gfx(struct chunk_t *);
extern void chunk_free(struct chunk_t *);
extern void chunk_init_block(struct chunk_t *, struct world_t *);
extern void chunk_update_gfx(struct chunk_t *, Vtx *);
extern struct chunk_t *chunk_init(s16 *);

#endif /* _CHUNK_H_ */
