#ifndef _PONG_H_
#define _PONG_H_

#include <ultra64.h>
#include <types.h>

#include "apphi.h"

struct pong_t
{
    struct controller_t *cnt;
    u8  update;
    u8  index;
    s32 score;
    f32 pos[2];
    f32 dir[2];
    f32 vel;
    f32 paddle;
};

extern void pong_init(struct pong_t *, struct controller_t *);
extern u8   pong_update(struct pong_t *);
extern void pong_draw(struct pong_t *);

#endif /* _PONG_H_ */
