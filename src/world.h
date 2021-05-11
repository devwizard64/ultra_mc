#ifndef _WORLD_H_
#define _WORLD_H_

#include <ultra64.h>
#include <types.h>

#include "apphi.h"
#include "player.h"

#define NIMAGE_W        32
#define NIMAGE_D        32
#define NIMAGE_LEN      (NIMAGE_W*NIMAGE_D)

#define HIMAGE_W        512
#define HIMAGE_D        512
#define HIMAGE_LEN      (HIMAGE_W*HIMAGE_D)

#define world_chunk(world, x, y, z) \
    (world)->chunk[(world)->siz*((world)->siz*(y)+(z))+(x)]

struct world_t
{
    struct config_t *config;
    struct player_t *player;
    Mtx    *mtx;
    Vtx    *vtx;
    u8     *h_image;
    struct chunk_t **chunk;
    u32     len;
    u16     siz;
    u16     rad;
};

extern void world_write_pos(s16 *, u8 *, f32 *);
extern void world_init_chunk(struct world_t *, u32);
extern void world_free_chunk(struct world_t *);
extern void world_init(struct world_t *, struct config_t *, struct player_t *);
extern void world_update(struct world_t *);
extern void world_update_chunk(struct world_t *);
extern void world_draw(struct world_t *);

#endif /* _WORLD_H_ */
