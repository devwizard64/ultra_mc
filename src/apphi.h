#ifndef _APPHI_H_
#define _APPHI_H_

#include <ultra64.h>

#include <types.h>

#include "menu.h"
#include "pong.h"
#include "player.h"
#include "world.h"

struct controller_t
{
    /* 0x00 */  OSContStatus *status;
    /* 0x04 */  OSContPad *pad;
    /* 0x08 */  f32     stick[2];
    /* 0x10 */  f32     mag;
    /* 0x14 */  u16     held;
    /* 0x16 */  u16     down;
};  /* 0x18 */

struct config_t
{
    union
    {
        struct
        {
            /* 0x00 */  u16 sum;
            /* 0x02 */  u8  v_mode:1;
            /* 0x02 */  u8  v_buf:1;
            /* 0x02 */  u8  v_aa:2;
            /* 0x02 */  u8  v_pm:1;
            /* 0x02 */  u8  v_clip:3;
            /* 0x03 */  u8  v_x;
            /* 0x04 */  u8  v_y;
            /* 0x05 */  u8  w_rad;
        };  /* 0x06 */
        u64 pad[1];
    };
};

extern OSThread thread_video;

extern struct controller_t controller_table[2];
extern struct config_t app_config;
extern struct menu_t app_menu;
extern struct pong_t app_pong;
extern struct player_t app_player;
extern struct world_t app_world;

extern void input_update(void);
extern void config_write(struct config_t *);
extern void apphi_main(void *);

#endif /* _APPHI_H_ */
