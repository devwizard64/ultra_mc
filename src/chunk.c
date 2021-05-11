#include <ultra64.h>

#include "mem.h"
#include "printf.h"
#include "world.h"
#include "chunk.h"
#include "block_gfx.h"

struct chunk_t *chunk_seek(struct chunk_t *chunk, s32 x, s32 y, s32 z)
{
    while (chunk != NULL && y < chunk->pos[1])
    {
        chunk = chunk->yl;
    }
    while (chunk != NULL && y > chunk->pos[1])
    {
        chunk = chunk->yh;
    }
    while (chunk != NULL && z < chunk->pos[2])
    {
        chunk = chunk->zl;
    }
    while (chunk != NULL && z > chunk->pos[2])
    {
        chunk = chunk->zh;
    }
    while (chunk != NULL && x < chunk->pos[0])
    {
        chunk = chunk->xl;
    }
    while (chunk != NULL && x > chunk->pos[0])
    {
        chunk = chunk->xh;
    }
    return chunk;
}

void chunk_block_write(struct chunk_t *chunk, u32 x, u32 y, u32 z, u32 block)
{
    chunk->block_xyz(x, y, z) = block;
    chunk->flag |= CHUNK_FLAG_INIT_GFX;
    if (x == 0 && chunk->xl != NULL)
    {
        chunk->xl->flag |= CHUNK_FLAG_INIT_GFX;
    }
    if (x == CHUNK_W-1 && chunk->xh != NULL)
    {
        chunk->xh->flag |= CHUNK_FLAG_INIT_GFX;
    }
    if (y == 0 && chunk->yl != NULL)
    {
        chunk->yl->flag |= CHUNK_FLAG_INIT_GFX;
    }
    if (y == CHUNK_H-1 && chunk->yh != NULL)
    {
        chunk->yh->flag |= CHUNK_FLAG_INIT_GFX;
    }
    if (z == 0 && chunk->zl != NULL)
    {
        chunk->zl->flag |= CHUNK_FLAG_INIT_GFX;
    }
    if (z == CHUNK_D-1 && chunk->zh != NULL)
    {
        chunk->zh->flag |= CHUNK_FLAG_INIT_GFX;
    }
}

void chunk_free_gfx(struct chunk_t *chunk)
{
    u32 i;
    for (i = 0; i < BLOCK_MTL_LEN; i++)
    {
        if (chunk->gfx[i] != NULL)
        {
            mem_free_gfx(chunk->gfx[i]);
            chunk->gfx[i] = NULL;
        }
    }
    chunk->flag |= CHUNK_FLAG_INIT_GFX;
}

void chunk_free(struct chunk_t *chunk)
{
    if (chunk->block != NULL)
    {
        mem_free(chunk->block);
    }
    chunk_free_gfx(chunk);
    mem_free(chunk);
}

void chunk_init_block(struct chunk_t *chunk, struct world_t *world)
{
    u8 *block;
    s32 y;
    s32 z;
    s32 x;
    u8 *h_image;
    block = mem_alloc(sizeof(*chunk->block) * CHUNK_LEN);
    if (block == NULL)
    {
        return;
    }
    y = CHUNK_H*chunk->pos[1];
    z = (u32)(CHUNK_D*chunk->pos[2]) % HIMAGE_D;
    x = (u32)(CHUNK_W*chunk->pos[0]) % HIMAGE_W;
    h_image = &world->h_image[HIMAGE_W*z+x];
    for (z = 0; z < CHUNK_D; z++, h_image += HIMAGE_W-CHUNK_W)
    {
        for (x = 0; x < CHUNK_W; x++, h_image++)
        {
            s32 h = *h_image + 64;
            s32 s = h * 64/0x400;
            s32 d = h * 80/0x400;
            u32 i;
            for (i = 0; i < CHUNK_H; i++)
            {
                s32 c = y+i;
                s32 b;
                if (c < 0)
                {
                    b = BLOCK_NULL;
                }
                else if (c == 0)
                {
                    b = BLOCK_BEDROCK;
                }
                else if (c < s)
                {
                    b = BLOCK_STONE;
                }
                else if (c < d)
                {
                    b = BLOCK_DIRT;
                }
                else if (c == d)
                {
                    b = BLOCK_GRASS;
                }
                else
                {
                    b = BLOCK_NULL;
                }
                block_xyz(x, i, z) = b;
            }
        }
    }
    chunk->block = block;
}

#define BLOCK_FLAG_XL 0x01
#define BLOCK_FLAG_XH 0x02
#define BLOCK_FLAG_ZL 0x04
#define BLOCK_FLAG_ZH 0x08
#define BLOCK_FLAG_YL 0x10
#define BLOCK_FLAG_YH 0x20

#define DRAW(s, k, d, v, x, y, z, i)                                           \
{                                                                              \
    b = (d) == (v) ? chunk->s->block_xyz(x, y, z) : block[i];                  \
    if (b == BLOCK_NULL)                                                       \
    {                                                                          \
        *f |= BLOCK_FLAG_##k;                                                  \
        *g += sizeof(*gfx[0]);                                                 \
    }                                                                          \
}
static void chunk_init_gfx(struct chunk_t *chunk, Vtx *vtx)
{
    Gfx   *gfx[BLOCK_MTL_LEN];
    size_t gfx_size[BLOCK_MTL_LEN];
    u8    *flag;
    u8    *block;
    u8    *f;
    u32    y;
    u32    z;
    u32    x;
    flag = mem_alloc(sizeof(*flag) * CHUNK_LEN);
    if (flag == NULL)
    {
        return;
    }
    for (y = 0; y < BLOCK_MTL_LEN; y++)
    {
        gfx[y] = 0;
        gfx_size[y] = 0;
    }
    for (block = chunk->block, f = flag, y = 0; y < CHUNK_H; y++)
    {
        for (z = 0; z < CHUNK_D; z++)
        {
            for (x = 0; x < CHUNK_W; x++, block++, f++)
            {
                u8 b;
                *f = 0;
                b = *block;
                if (b != BLOCK_NULL)
                {
                    size_t *g = &gfx_size[block_gfx_table[b].mtl];
                    DRAW(xl, XL, x, 0, CHUNK_W-1, y, z,               -1);
                    DRAW(xh, XH, x, CHUNK_W-1, 0, y, z,                1);
                    DRAW(zl, ZL, z, 0, x, y, CHUNK_D-1,         -CHUNK_W);
                    DRAW(zh, ZH, z, CHUNK_D-1, x, y, 0,          CHUNK_W);
                    DRAW(yl, YL, y, 0, x, CHUNK_H-1, z, -CHUNK_D*CHUNK_W);
                    DRAW(yh, YH, y, CHUNK_H-1, x, 0, z,  CHUNK_D*CHUNK_W);
                    if (*f != 0)
                    {
                        *g += sizeof(*gfx[0]);
                    }
                }
            }
        }
    }
    for (y = 0; y < BLOCK_MTL_LEN; y++)
    {
        if (gfx_size[y] != 0)
        {
            chunk->gfx[y] = gfx[y] = mem_alloc(gfx_size[y] + sizeof(*gfx[0])*3);
            if (gfx[y] == NULL)
            {
                chunk_free_gfx(chunk);
                goto end;
            }
            gSPVertex(gfx[y]++, vtx_chunk, 8, 0);
            gSPCullDisplayList(gfx[y]++, 0, 7);
        }
    }
    for (block = chunk->block, f = flag, y = 0; y < CHUNK_H; y++)
    {
        for (z = 0; z < CHUNK_D; z++)
        {
            for (x = 0; x < CHUNK_W; x++, block++, f++, vtx += 4)
            {
                if (*f != 0)
                {
                    const struct block_gfx_t *b = &block_gfx_table[*block];
                    Gfx *g = gfx[b->mtl];
                    gSPVertex(g++, vtx, 8, 0);
                    if (*f & BLOCK_FLAG_XL)
                    {
                        gSPDisplayList(g++, b->xl);
                    }
                    if (*f & BLOCK_FLAG_XH)
                    {
                        gSPDisplayList(g++, b->xh);
                    }
                    if (*f & BLOCK_FLAG_ZL)
                    {
                        gSPDisplayList(g++, b->zl);
                    }
                    if (*f & BLOCK_FLAG_ZH)
                    {
                        gSPDisplayList(g++, b->zh);
                    }
                    if (*f & BLOCK_FLAG_YL)
                    {
                        gSPDisplayList(g++, b->yl);
                    }
                    if (*f & BLOCK_FLAG_YH)
                    {
                        gSPDisplayList(g++, b->yh);
                    }
                    gfx[b->mtl] = g;
                }
            }
            if ((z & 1) == 0)
            {
                vtx = (Vtx *)((u8 *)vtx - sizeof(*vtx)/2 * (2*4*CHUNK_W-1));
            }
            else
            {
                vtx = (Vtx *)((u8 *)vtx + sizeof(*vtx)/2 * (2*4-1));
            }
        }
    }
    for (y = 0; y < BLOCK_MTL_LEN; y++)
    {
        if (gfx[y] != NULL)
        {
            gSPEndDisplayList(gfx[y]++);
        }
    }
    chunk->flag &= ~CHUNK_FLAG_INIT_GFX;
end:
    mem_free(flag);
}

void chunk_update_gfx(struct chunk_t *chunk, Vtx *vtx)
{
    if
    (
        chunk->block == NULL ||
        chunk->yl == NULL || chunk->yl->block == NULL ||
        chunk->yh == NULL || chunk->yh->block == NULL ||
        chunk->zl == NULL || chunk->zl->block == NULL ||
        chunk->zh == NULL || chunk->zh->block == NULL ||
        chunk->xl == NULL || chunk->xl->block == NULL ||
        chunk->xh == NULL || chunk->xh->block == NULL
    )
    {
        chunk_free_gfx(chunk);
    }
    else if (chunk->flag & CHUNK_FLAG_INIT_GFX)
    {
        chunk_free_gfx(chunk);
        chunk_init_gfx(chunk, vtx);
    }
}
#undef DRAW

struct chunk_t *chunk_init(s16 *pos)
{
    struct chunk_t *chunk = mem_alloc(sizeof(*chunk));
    if (chunk != NULL)
    {
        u32 i;
        chunk->xl = NULL;
        chunk->xh = NULL;
        chunk->yl = NULL;
        chunk->yh = NULL;
        chunk->zl = NULL;
        chunk->zh = NULL;
        chunk->flag = CHUNK_FLAG_INIT_GFX;
        chunk->pos[0] = pos[0];
        chunk->pos[1] = pos[1];
        chunk->pos[2] = pos[2];
        chunk->block = NULL;
        for (i = 0; i < BLOCK_MTL_LEN; i++)
        {
            chunk->gfx[i] = NULL;
        }
    }
    return chunk;
}
