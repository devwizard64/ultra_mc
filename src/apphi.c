#include <ultra64.h>

#include <types.h>

#include "main.h"
#include "apphi.h"
#include "video.h"
#include "applo.h"
#include "mem.h"
#include "menu.h"
#include "pong.h"
#include "player.h"
#include "world.h"

static u64 stack_video[0x2000/sizeof(u64)];
static u64 stack_applo[0x2000/sizeof(u64)];
OSThread thread_video;
static OSThread thread_applo;

static OSMesgQueue mq_apphi;
static OSMesgQueue mq_apphi_si;
static OSMesg msg_apphi[1];
static OSMesg msg_apphi_si[1];
static OSContStatus contstatus_table[4];
static OSContPad contpad_table[4];
static u8 eeprom_status;
struct controller_t controller_table[2];
struct config_t app_config;
struct menu_t app_menu;
struct pong_t app_pong;
struct player_t app_player;
struct world_t app_world;

static void input_init(void)
{
    u32 c;
    u32 i;
    u8  flags;
    osCreateMesgQueue(MQ(apphi_si));
    osSetEventMesg(OS_EVENT_SI, &mq_apphi_si, (OSMesg)0);
    osContInit(&mq_apphi_si, &flags, contstatus_table);
    for (c = 0, i = 0; i < 4 && c < lenof(controller_table); i++, flags >>= 1)
    {
        if (flags & 0x01)
        {
            struct controller_t *cnt = &controller_table[c];
            cnt->status = &contstatus_table[i];
            cnt->pad = &contpad_table[i];
            c++;
        }
    }
}

void input_update(void)
{
    u32 i;
    osContStartReadData(&mq_apphi_si);
    osRecvMesg(&mq_apphi_si, NULL, OS_MESG_BLOCK);
    osContGetReadData(contpad_table);
    for (i = 0; i < lenof(controller_table); i++)
    {
        struct controller_t *cnt = &controller_table[i];
        OSContPad *pad = cnt->pad;
        if (pad != NULL)
        {
            cnt->down = pad->button & (pad->button^cnt->held);
            cnt->held = pad->button;
            if (pad->stick_x <= -8)
            {
                cnt->stick[0] = pad->stick_x+6;
            }
            else if (pad->stick_x >= 8)
            {
                cnt->stick[0] = pad->stick_x-6;
            }
            else
            {
                cnt->stick[0] = 0;
            }
            if (pad->stick_y <= -8)
            {
                cnt->stick[1] = pad->stick_y+6;
            }
            else if (pad->stick_y >= 8)
            {
                cnt->stick[1] = pad->stick_y-6;
            }
            else
            {
                cnt->stick[1] = 0;
            }
            cnt->mag = sqrtf(
                cnt->stick[0]*cnt->stick[0] + cnt->stick[1]*cnt->stick[1]
            );
            if (cnt->mag > 64)
            {
                f32 d = 64 / cnt->mag;
                cnt->stick[0] *= d;
                cnt->stick[1] *= d;
                cnt->mag = 64;
            }
        }
    }
}

static u16 config_sum(struct config_t *cfg)
{
    u16 *src = (u16 *)cfg;
    u32  cnt = sizeof(*cfg)/sizeof(*src) - 1;
    u32  sum = 'M' << 8 | 'K';
    do
    {
        sum += src[1];
        src++;
        cnt--;
    }
    while (cnt != 0);
    return sum;
}

static void config_read(struct config_t *cfg)
{
    if (eeprom_status != 0)
    {
        osEepromLongRead(&mq_apphi_si, 0, (u8 *)cfg, sizeof(*cfg));
    }
    else
    {
        memcpy(cfg, osAppNMIBuffer, sizeof(*cfg));
    }
    if (cfg->sum != config_sum(cfg))
    {
        bzero(cfg, sizeof(*cfg));
        config_write(cfg);
    }
    video_sl = cfg->v_mode;
    video_sr = video_sl ^ 1;
    video_tb = cfg->v_buf ^ 1;
}

void config_write(struct config_t *cfg)
{
    cfg->sum = config_sum(cfg);
    if (eeprom_status != 0)
    {
        osEepromLongWrite(&mq_apphi_si, 0, (u8 *)cfg, sizeof(*cfg));
    }
    else
    {
        memcpy(osAppNMIBuffer, cfg, sizeof(*cfg));
    }
}

static void config_init(struct config_t *cfg)
{
    eeprom_status = osEepromProbe(&mq_apphi_si);
    config_read(cfg);
}

static void apphi_init(void)
{
    osCreateMesgQueue(MQ(apphi));
    input_init();
    config_init(&app_config);
    mem_init();
    video_init();
    menu_init(&app_menu, &controller_table[0], &app_config, &app_world);
    pong_init(&app_pong, &controller_table[0]);
    player_init(&app_player, &controller_table[0], &app_world);
    world_init(&app_world, &app_config, &app_player);
    thread_create(video, 6, 0x10);
    thread_create(applo, 7, 0x08);
}

static void apphi_update(void)
{
    input_update();
    if (!menu_update(&app_menu) && !pong_update(&app_pong))
    {
        player_update(&app_player);
        world_update(&app_world);
    }
    osRecvMesg(&mq_apphi_vi, NULL, OS_MESG_BLOCK);
}

void apphi_main(unused void *arg)
{
    apphi_init();
    while (true)
    {
        apphi_update();
    }
}
