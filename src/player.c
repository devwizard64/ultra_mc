#include <ultra64.h>

#include "video.h"
#include "mem.h"
#include "math.h"
#include "printf.h"
#include "player.h"
#include "world.h"
#include "chunk.h"

static void player_update_world_pos(struct player_t *player)
{
    world_write_pos(player->c_pos, player->b_pos, player->pos);
    player->cam[0] = BLOCK_W*CHUNK_W*player->c_pos[0] - player->pos[0];
    player->cam[1] = BLOCK_H*CHUNK_H*player->c_pos[1] - player->pos[1];
    player->cam[2] = BLOCK_D*CHUNK_D*player->c_pos[2] - player->pos[2];
    player->cam[1] -= 2*BLOCK_H;
}

void player_init(
    struct player_t *player, struct controller_t *cnt, struct world_t *world
)
{
    player->cnt = cnt;
    player->world = world;
    player->chunk = NULL;
    player->pos[0] = BLOCK_W*4;
    player->pos[1] = BLOCK_H*20;
    player->pos[2] = BLOCK_D*4;
    player->vel[0] = 0.0F;
    player->vel[1] = 0.0F;
    player->vel[2] = 0.0F;
    player->rot[0] = 0x0000;
    player->rot[1] = 0x0000;
    player->rot[2] = 0x0000;
    player_update_world_pos(player);
}

static void player_update_vel(struct player_t *player)
{
    f32 x = 0.25F * player->cnt->stick[0];
    f32 y = 0.25F * player->cnt->stick[1];
    u16 held = player->cnt->held;
    player->vel[0] = cos(player->rot[1])*x + sin(player->rot[1])*y;
    player->vel[2] = sin(player->rot[1])*x - cos(player->rot[1])*y;
    if (held & B_BUTTON)
    {
        player->vel[0] *= 8.0F;
        player->vel[2] *= 8.0F;
    }
    if (player->vel[1] > 0.0F && !(held & A_BUTTON))
    {
        player->vel[1] *= 1.0F/2.0F;
    }
    if (player->pos[1] > player->ground_y)
    {
        player->vel[1] -= 1.0F;
        if (player->vel[1] < -37.5F)
        {
            player->vel[1] = -37.5F;
        }
    }
}

static void player_update_rot(struct player_t *player)
{
    u16 held = player->cnt->held;
    if (held & D_CBUTTONS)
    {
        player->rot[0] -= 0x0100;
    }
    if (held & U_CBUTTONS)
    {
        player->rot[0] += 0x0100;
    }
    if (held & L_CBUTTONS)
    {
        player->rot[1] -= 0x0100;
    }
    if (held & R_CBUTTONS)
    {
        player->rot[1] += 0x0100;
    }
    if (player->rot[0] < -0x3800)
    {
        player->rot[0] = -0x3800;
    }
    if (player->rot[0] >  0x3800)
    {
        player->rot[0] =  0x3800;
    }
}

static void player_update_pos(struct player_t *player)
{
#if 0
    s64 x;
    s64 z;
#endif
    player->pos[0] += player->vel[0];
    player->pos[1] += player->vel[1];
    player->pos[2] += player->vel[2];
    if (player->pos[1] < -32*BLOCK_H)
    {
        player->pos[1] = -32*BLOCK_H;
    }
    if (player->pos[1] >  127*BLOCK_H)
    {
        player->pos[1] =  127*BLOCK_H;
    }
#if 0
    x = (1.0F/BLOCK_W) * player->pos[0];
    z = (1.0F/BLOCK_D) * player->pos[2];
    if (player->pos[0] < (f32)(-0x7FFFFFFFLL*BLOCK_W))
    {
        player->pos[0] = (f32)(-0x7FFFFFFFLL*BLOCK_W);
    }
    if (player->pos[0] > (f32)( 0x7FFFFFFFLL*BLOCK_W))
    {
        player->pos[0] = (f32)( 0x7FFFFFFFLL*BLOCK_W);
    }
    if (player->pos[2] < (f32)(-0x7FFFFFFFLL*BLOCK_W))
    {
        player->pos[2] = (f32)(-0x7FFFFFFFLL*BLOCK_W);
    }
    if (player->pos[2] > (f32)( 0x7FFFFFFFLL*BLOCK_W))
    {
        player->pos[2] = (f32)( 0x7FFFFFFFLL*BLOCK_W);
    }
#endif
}

static void player_update_ground_y(struct player_t *player)
{
    struct chunk_t *chunk;
    s32 x;
    s32 y;
    s32 z;
    player->ground_y = BLOCK_H * -32;
    chunk = player->chunk;
    x = player->b_pos[0];
    y = (s32)player->pos[1] + BLOCK_H/2;
    z = player->b_pos[2];
    while (chunk != NULL)
    {
        s32 block_y = (u32)y/BLOCK_H % CHUNK_H;
        if
        (
            chunk->block != NULL &&
            chunk->block_xyz(x, block_y, z) != BLOCK_NULL
        )
        {
            player->ground_y = BLOCK_H * (CHUNK_H*chunk->pos[1] + block_y + 1);
            break;
        }
        y -= BLOCK_H/8;
        if (y < 0)
        {
            chunk = chunk->yl;
            y += BLOCK_H*CHUNK_H;
        }
    }
}

static void player_update_gravity(struct player_t *player)
{
    if (player->pos[1] <= player->ground_y)
    {
        player->pos[1] = player->ground_y;
        player->vel[1] = 0.0F;
        if (player->cnt->down & A_BUTTON)
        {
            player->vel[1] = 20.0F;
        }
    }
}

static void player_update_look(struct player_t *player)
{
    struct chunk_t *chunk_r;
    struct chunk_t *chunk_w;
    f32 pos[3];
    f32 x;
    f32 y;
    f32 z;
    s32 i;
    player->chunk_r = player->chunk_w = NULL;
    chunk_r = player->chunk;
    chunk_w = NULL;
    pos[0] = player->pos[0];
    pos[1] = player->pos[1] + 2*BLOCK_H;
    pos[2] = player->pos[2];
    world_write_pos(player->c_w, player->b_w, pos);
    z = cos(player->rot[0]);
    x =  (BLOCK_W/8.0F) * z*sin(player->rot[1]);
    y = -(BLOCK_H/8.0F) *   sin(player->rot[0]);
    z = -(BLOCK_D/8.0F) * z*cos(player->rot[1]);
    for (i = 0; i < 8*5; i++)
    {
        u8 block;
        pos[0] += x;
        pos[1] += y;
        pos[2] += z;
        world_write_pos(player->c_r, player->b_r, pos);
        chunk_r = chunk_seek(
            chunk_r,
            player->c_r[0],
            player->c_r[1],
            player->c_r[2]
        );
        if (chunk_r == NULL || chunk_r->block == NULL)
        {
            break;
        }
        block = chunk_r->block_xyz(
            player->b_r[0],
            player->b_r[1],
            player->b_r[2]
        );
        if (block != BLOCK_NULL)
        {
            player->chunk_r = chunk_r;
            player->chunk_w = chunk_w;
            break;
        }
        chunk_w = chunk_r;
        *player->c_w = *player->c_r;
        *player->b_w = *player->b_r;
    }
}

static void player_update_block(struct player_t *player)
{
    if (player->cnt->down & Z_TRIG)
    {
        if (player->chunk_r != NULL)
        {
            chunk_block_write(
                player->chunk_r,
                player->b_r[0],
                player->b_r[1],
                player->b_r[2],
                BLOCK_NULL
            );
        }
    }
    if (player->cnt->down & R_TRIG)
    {
        if (player->chunk_w != NULL)
        {
            chunk_block_write(
                player->chunk_w,
                player->b_w[0],
                player->b_w[1],
                player->b_w[2],
                BLOCK_COBBLESTONE
            );
        }
    }
}

void player_update(struct player_t *player)
{
    struct world_t *world = player->world;
    player->chunk = world_chunk(world, world->rad, world->rad, world->rad);
    player_update_vel(player);
    player_update_rot(player);
    player_update_pos(player);
    player_update_ground_y(player);
    player_update_gravity(player);
    player_update_world_pos(player);
    player_update_look(player);
    player_update_block(player);
}

void player_draw(struct player_t *player)
{
    Mtx *mtx;
    u16 persp_norm;
    mtx = mem_alloc_gfx(sizeof(*mtx));
    guPerspective(mtx, &persp_norm, 70.0F, video_aspect, 64.0F, 16384.0F, 1.0F);
    gSPPerspNormalize(video_gfx++, persp_norm);
    gSPMatrix(video_gfx++, mtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    mtx = mem_alloc_gfx(sizeof(*mtx));
    guRotate(mtx, (180.0F/32768.0F) * player->rot[0], 1.0F, 0.0F, 0.0F);
    gSPMatrix(video_gfx++, mtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    mtx = mem_alloc_gfx(sizeof(*mtx));
    guRotate(mtx, (180.0F/32768.0F) * player->rot[1], 0.0F, 1.0F, 0.0F);
    gSPMatrix(video_gfx++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    mtx = mem_alloc_gfx(sizeof(*mtx));
    guTranslate(mtx, player->cam[0], player->cam[1], player->cam[2]);
    gSPMatrix(video_gfx++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
    printf(
        "wx:%d\n"
        "wy:%d\n"
        "wz:%d\n",
        (s32)player->pos[0], (s32)player->pos[1], (s32)player->pos[2]
    );
}
