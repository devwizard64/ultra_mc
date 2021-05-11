#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <ultra64.h>
#include <types.h>

#include "apphi.h"
#include "world.h"

struct player_t
{
    /* 0x00 */  struct controller_t *cnt;
    /* 0x04 */  struct world_t *world;
    /* 0x08 */  struct chunk_t *chunk;
    /* 0x0C */  struct chunk_t *chunk_r;
    /* 0x10 */  struct chunk_t *chunk_w;
    /* 0x14 */  f32 pos[3];
    /* 0x20 */  f32 cam[3];
    /* 0x2C */  f32 vel[3];
    /* 0x38 */  f32 vel_f;
    /* 0x3C */  f32 vel_h[2];
    /* 0x44 */  s16 ground_y;
    /* 0x46 */  s16 rot[3];
    /* 0x4C */  s16 c_pos[3];
    /* 0x52 */  s16 c_r[3];
    /* 0x58 */  s16 c_w[3];
    /* 0x5E */  u8  b_pos[3];
    /* 0x61 */  u8  b_r[3];
    /* 0x64 */  u8  b_w[3];
};  /* 0x67 */

extern void player_init(
    struct player_t *, struct controller_t *, struct world_t *
);
extern void player_update(struct player_t *);
extern void player_draw(struct player_t *);

#endif /* _PLAYER_H_ */
