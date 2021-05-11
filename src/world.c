#include <ultra64.h>

#include "gbi_ext.h"

#include "main.h"
#include "video.h"
#include "applo.h"
#include "mem.h"
#include "math.h"
#include "printf.h"
#include "player.h"
#include "world.h"
#include "chunk.h"
#include "block_gfx.h"

static Gfx gfx_world_init[] =
{
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, HIMAGE_W, HIMAGE_D),
    gsDPSetOtherMode(
        G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP |
        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE |
        G_PM_1PRIMITIVE,
        G_AC_NONE | G_ZS_PIXEL | G_RM_XLU_SURF | G_RM_XLU_SURF2
    ),
    gsDPSetColorImage(G_IM_FMT_I, G_IM_SIZ_8b, HIMAGE_W, NULL),
    gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_16b, 1, NULL),
    gsDPLoadTextureBlockN(
        G_IM_FMT_I, G_IM_SIZ_8b, NIMAGE_W, NIMAGE_D, 0,
        G_TX_WRAP | G_TX_NOMIRROR, G_TX_WRAP | G_TX_NOMIRROR, 5, 5, G_TX_NOLOD,
        G_TX_NOLOD
    ),
    gsDPSetCombineLERP(
        0, 0, 0, TEXEL0, 0, 0, 0, 1,
        0, 0, 0, TEXEL0, 0, 0, 0, 1
    ),
    gsSPTextureRectangle(
        0 << 2, 0 << 2, HIMAGE_W << 2, HIMAGE_D << 2, G_TX_RENDERTILE,
        0, 0, 0x0040, 0x0040
    ),
    gsDPSetEnvColor(0x00, 0x00, 0x00, 0x7F),
    gsDPSetCombineLERP(
        0, 0, 0, 1, TEXEL0, 0, ENVIRONMENT, 0,
        0, 0, 0, 1, TEXEL0, 0, ENVIRONMENT, 0
    ),
    gsSPTextureRectangle(
        0 << 2, 0 << 2, HIMAGE_W << 2, HIMAGE_D << 2, G_TX_RENDERTILE,
        8176, 12304, 0x0040, 0xFFC0
    ),
    gsDPFullSync(),
    gsSPEndDisplayList(),
};

void world_write_pos(s16 *c_pos, u8 *b_pos, f32 *pos)
{
    u32 x = (u32)pos[0] / BLOCK_W;
    u32 y = (u32)pos[1] / BLOCK_H;
    u32 z = (u32)pos[2] / BLOCK_D;
    c_pos[0] = x / CHUNK_W;
    c_pos[1] = y / CHUNK_H;
    c_pos[2] = z / CHUNK_D;
    b_pos[0] = x % CHUNK_W;
    b_pos[1] = y % CHUNK_H;
    b_pos[2] = z % CHUNK_D;
}

static void world_update_shift(struct world_t *world)
{
    struct chunk_t **chunk;
    size_t size;
    s32 cx;
    s32 cy;
    s32 cz;
    u32 i;
    size = sizeof(*world->chunk) * world->len;
    chunk = mem_alloc(size);
    if (chunk == NULL)
    {
        return;
    }
    memcpy(chunk, world->chunk, size);
    bzero(world->chunk, size);
    cx = world->player->c_pos[0] - world->rad;
    cy = world->player->c_pos[1] - world->rad;
    cz = world->player->c_pos[2] - world->rad;
    for (i = 0; i < world->len; i++)
    {
        struct chunk_t *c = chunk[i];
        if (c != NULL)
        {
            u32 iy;
            u32 iz;
            u32 ix;
            s32 y;
            s32 z;
            s32 x;
            for (iy = 0, y = cy; iy < world->siz; iy++, y++)
            {
                for (iz = 0, z = cz; iz < world->siz; iz++, z++)
                {
                    for (ix = 0, x = cx; ix < world->siz; ix++, x++)
                    {
                        s32 xx = c->pos[0];
                        s32 yy = c->pos[1];
                        s32 zz = c->pos[2];
                        if (xx == x && yy == y && zz == z)
                        {
                            world_chunk(world, ix, iy, iz) = c;
                            goto end;
                        }
                    }
                }
            }
            chunk_free(c);
        end:;
        }
    }
    mem_free(chunk);
}

static void world_update_link(struct world_t *world)
{
    struct chunk_t **chunk;
    u32 siz;
    u32 y;
    u32 z;
    u32 x;
    siz = world->siz;
    for (chunk = world->chunk, y = 0; y < siz; y++)
    {
        for (z = 0; z < siz; z++)
        {
            for (x = 0; x < siz; x++, chunk++)
            {
                if (chunk[0] == NULL)
                {
                    continue;
                }
                chunk[0]->yl = y !=     0 ? chunk[-siz*siz] : NULL;
                chunk[0]->yh = y != siz-1 ? chunk[ siz*siz] : NULL;
                chunk[0]->zl = z !=     0 ? chunk[    -siz] : NULL;
                chunk[0]->zh = z != siz-1 ? chunk[     siz] : NULL;
                chunk[0]->xl = x !=     0 ? chunk[      -1] : NULL;
                chunk[0]->xh = x != siz-1 ? chunk[       1] : NULL;
            }
        }
    }
}

void world_update(struct world_t *world)
{
    if (world->chunk != NULL)
    {
        struct chunk_t *chunk = world_chunk(
            world, world->rad, world->rad, world->rad
        );
        if (chunk != NULL)
        {
            s16 cx = chunk->pos[0];
            s16 cy = chunk->pos[1];
            s16 cz = chunk->pos[2];
            s16 px = world->player->c_pos[0];
            s16 py = world->player->c_pos[1];
            s16 pz = world->player->c_pos[2];
            if (cx != px || cy != py || cz != pz)
            {
                world_update_shift(world);
            }
        }
        world_update_link(world);
    }
}

void world_update_chunk(struct world_t *world)
{
    s32 cx;
    s32 cy;
    s32 cz;
    u32 i;
    u32 y;
    u32 z;
    u32 x;
    s16 pos[3];
    cx = world->player->c_pos[0] - world->rad;
    cy = world->player->c_pos[1] - world->rad;
    cz = world->player->c_pos[2] - world->rad;
    i = 0;
    for (y = 0, pos[1] = cy; y < world->siz; y++, pos[1]++)
    {
        for (z = 0, pos[2] = cz; z < world->siz; z++, pos[2]++)
        {
            for (x = 0, pos[0] = cx; x < world->siz; x++, pos[0]++)
            {
                struct chunk_t *chunk;
                if (world->chunk == NULL)
                {
                    return;
                }
                chunk = world->chunk[i];
                if (chunk == NULL)
                {
                    chunk = world->chunk[i] = chunk_init(pos);
                }
                if (chunk != NULL)
                {
                    if (chunk->block == NULL)
                    {
                        chunk_init_block(chunk, world);
                    }
                    chunk_update_gfx(chunk, world->vtx);
                }
                i++;
            }
        }
    }
}

static void world_init_mtx(struct world_t *world)
{
    Mtx *mtx;
    f32  ty;
    f32  tz;
    f32  tx;
    u32  y;
    u32  z;
    u32  x;
    world->mtx = mtx = mem_alloc(sizeof(*mtx) * world->len);
    if (mtx == NULL)
    {
        return;
    }
    ty = -BLOCK_H*CHUNK_H*world->rad;
    for (y = 0; y < world->siz; y++, ty += BLOCK_H*CHUNK_H)
    {
        tz = -BLOCK_D*CHUNK_D*world->rad;
        for (z = 0; z < world->siz; z++, tz += BLOCK_D*CHUNK_D)
        {
            tx = -BLOCK_W*CHUNK_W*world->rad;
            for (x = 0; x < world->siz; x++, tx += BLOCK_W*CHUNK_W)
            {
                guTranslate(mtx++, tx, ty, tz);
            }
        }
    }
}

static void world_init_vtx(struct world_t *world)
{
    Vtx *vtx;
    s32  vy;
    s32  vz;
    s32  vx;
    u32  y;
    u32  z;
    u32  x;
    world->vtx = vtx = mem_alloc(sizeof(*vtx)*2 * CHUNK_H*CHUNK_D*(CHUNK_W+1));
    for (vy = 0, y = 0; y < CHUNK_H; y++, vy += BLOCK_H)
    {
        for (vz = 0, z = 0; z < CHUNK_D; z++, vz += BLOCK_D)
        {
            for (vx = 0, x = 0; x < CHUNK_W+1; x++, vx += BLOCK_W, vtx += 4)
            {
                vtx[0].v.ob[0] = vx;
                vtx[0].v.ob[1] = vy;
                vtx[0].v.ob[2] = vz;
                vtx[1].v.ob[0] = vx;
                vtx[1].v.ob[1] = vy;
                vtx[1].v.ob[2] = vz + BLOCK_D;
                vtx[2].v.ob[0] = vx;
                vtx[2].v.ob[1] = vy + BLOCK_H;
                vtx[2].v.ob[2] = vz;
                vtx[3].v.ob[0] = vx;
                vtx[3].v.ob[1] = vy + BLOCK_H;
                vtx[3].v.ob[2] = vz + BLOCK_D;
            }
            if ((z & 1) == 0)
            {
                vtx = (Vtx *)((u8 *)vtx - sizeof(*vtx)/2 * (2*4*(CHUNK_W+1)-1));
            }
            else
            {
                vtx = (Vtx *)((u8 *)vtx - sizeof(*vtx)/2);
            }
        }
    }
}

static void world_init_block(struct world_t *world)
{
    u8 *tmp;
    u32 i;
    world->h_image = mem_alloc(sizeof(*world->h_image)*HIMAGE_LEN);
    osInvalDCache(world->h_image, sizeof(*world->h_image)*HIMAGE_LEN);
    tmp = mem_alloc(sizeof(*tmp)*NIMAGE_LEN);
    i = 0;
    do
    {
        u16 r = rng_u16();
        tmp[i++] = r >> 8;
        tmp[i++] = r >> 0;
    }
    while (i < NIMAGE_LEN);
    gfx_world_init[2].words.w1 = (uintptr_t)world->h_image;
    gfx_world_init[3].words.w1 = (uintptr_t)tmp;
    video_draw(&video_task_table[1], gfx_world_init, sizeof(gfx_world_init));
    osRecvMesg(&mq_video_dp, NULL, OS_MESG_BLOCK);
    mem_free(tmp);
}

void world_init_chunk(struct world_t *world, u32 rad)
{
    size_t size;
    world->rad = rad;
    world->siz = 2*rad+1;
    world->len = world->siz * world->siz * world->siz;
    world_init_mtx(world);
    size = sizeof(*world->chunk) * world->len;
    world->chunk = mem_alloc(size);
    if (world->chunk == NULL)
    {
        return;
    }
    bzero(world->chunk, size);
    world_update(world);
}

void world_free_chunk(struct world_t *world)
{
    Mtx *mtx;
    struct chunk_t **chunk;
    u32 i;
    mtx = world->mtx;
    world->mtx = NULL;
    chunk = world->chunk;
    world->chunk = NULL;
    world->player->chunk_r = NULL;
    world->player->chunk_w = NULL;
    for (i = 0; i < world->len; i++)
    {
        if (chunk[i] != NULL)
        {
            chunk_free(chunk[i]);
        }
    }
    mem_free(chunk);
    mem_free(mtx);
}

void world_init(
    struct world_t *world, struct config_t *config, struct player_t *player
)
{
    world->config = config;
    world->player = player;
    world_init_vtx(world);
    world_init_block(world);
    world_init_chunk(world, 1 + world->config->w_rad);
}

void world_draw(struct world_t *world)
{
    u32 i;
    for (i = 0; i < BLOCK_MTL_LEN; i++)
    {
        u32 c;
        gSPDisplayList(video_gfx++, block_mtl_table[i]);
        for (c = 0; c < world->len; c++)
        {
            struct chunk_t *chunk;
            if (world->mtx == NULL || world->chunk == NULL)
            {
                return;
            }
            chunk = world->chunk[c];
            if (chunk != NULL && chunk->gfx[i] != NULL)
            {
                gSPMatrix(
                    video_gfx++, &world->mtx[c],
                    G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH
                );
                gSPDisplayList(video_gfx++, chunk->gfx[i]);
                gSPPopMatrix(video_gfx++, G_MTX_MODELVIEW);
            }
        }
    }
    if
    (
        world->player->chunk_r != NULL &&
        world->chunk != NULL && world->chunk[0] != NULL
    )
    {
        Mtx *mtx;
        u32 x;
        u32 y;
        u32 z;
        x = world->player->c_r[0] - world->chunk[0]->pos[0];
        y = world->player->c_r[1] - world->chunk[0]->pos[1];
        z = world->player->c_r[2] - world->chunk[0]->pos[2];
        mtx = &world->mtx[world->siz*(world->siz*y+z)+x];
        gSPMatrix(video_gfx++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        mtx = mem_alloc_gfx(sizeof(*mtx));
        x = world->player->b_r[0];
        y = world->player->b_r[1];
        z = world->player->b_r[2];
        guTranslate(mtx, BLOCK_W*x, BLOCK_H*y, BLOCK_D*z);
        gSPMatrix(video_gfx++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gSPDisplayList(video_gfx++, gfx_block_outline);
    }
    gDPSetTextureImage(
        video_gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320 << video_sl,
        video_cimg[video_cimg_dp]
    );
    gSPDisplayList(
        video_gfx++,
        video_sl == VIDEO_MODE_LO ? gfx_block_end_lo : gfx_block_end_hi
    );
}
