#ifndef _MENU_H_
#define _MENU_H_

#include <ultra64.h>
#include <types.h>

#include "apphi.h"
#include "world.h"

struct menu_t
{
    struct controller_t *cnt;
    struct config_t *config;
    struct world_t *world;
    u8  update;
    u8  index;
    const char *status;
    s32 option[8];
};

extern void menu_init(
    struct menu_t *, struct controller_t *, struct config_t *, struct world_t *
);
extern u8   menu_update(struct menu_t *);
extern void menu_draw(struct menu_t *);

#endif /* _MENU_H_ */
