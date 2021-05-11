#include <ultra64.h>

#include "gbi_ext.h"

#include "video.h"
#include "printf.h"
#include "menu.h"
#include "world.h"

struct menu_option_t
{
    const char *label;
    const char **table;
    s32 min;
    s32 max;
};

static const char *str_res_table[] =
{
    "320x240",
    "640x480",
};

static const char *str_buf_table[] =
{
    "triple",
    "double",
};

static const char *str_aa_table[] =
{
    "off",
    "low",
    "med",
    "high",
};

static const char *str_pm_table[] =
{
    "fast",
    "safe",
};

static const struct menu_option_t menu_option_table[] =
{
    {"resolution",     str_res_table, 0, lenof(str_res_table)-1},
    {"buffer mode",    str_buf_table, 0, lenof(str_buf_table)-1},
    {"anti-aliasing",  str_aa_table,  0, lenof(str_aa_table)-1},
    {"polygon mode",   str_pm_table,  0, lenof(str_pm_table)-1},
    {"clip ratio",     NULL,          1, 8},
    {"border x",       NULL,          0, 255},
    {"border y",       NULL,          0, 255},
    {"chunk distance", NULL,          1, 256},
};

static void menu_read(struct menu_t *menu)
{
    menu->option[0] = menu->config->v_mode;
    menu->option[1] = menu->config->v_buf;
    menu->option[2] = menu->config->v_aa;
    menu->option[3] = menu->config->v_pm;
    menu->option[4] = menu->config->v_clip + 1;
    menu->option[5] = menu->config->v_x;
    menu->option[6] = menu->config->v_y;
    menu->option[7] = menu->config->w_rad + 1;
}

static void menu_write(struct menu_t *menu)
{
    menu->config->v_mode = menu->option[0];
    menu->config->v_buf  = menu->option[1];
    menu->config->v_aa   = menu->option[2];
    menu->config->v_pm   = menu->option[3];
    menu->config->v_clip = menu->option[4] - 1;
    menu->config->v_x    = menu->option[5];
    menu->config->v_y    = menu->option[6];
    menu->config->w_rad  = menu->option[7] - 1;
    if (menu->index == 7)
    {
        world_free_chunk(menu->world);
        world_init_chunk(menu->world, 1 + menu->config->w_rad);
    }
}

void menu_init(
    struct menu_t *menu, struct controller_t *cnt,
    struct config_t *config, struct world_t *world
)
{
    menu->cnt    = cnt;
    menu->config = config;
    menu->world  = world;
    menu->update = false;
    menu->index  = 0;
    menu_read(menu);
}

u8 menu_update(struct menu_t *menu)
{
    if (menu->update)
    {
        s32 *option;
        const struct menu_option_t *menu_option;
        if (menu->cnt->down & U_JPAD)
        {
            if (menu->index > 0)
            {
                menu->index--;
            }
        }
        if (menu->cnt->down & D_JPAD)
        {
            if (menu->index < lenof(menu_option_table)-1)
            {
                menu->index++;
            }
        }
        option = &menu->option[menu->index];
        menu_option = &menu_option_table[menu->index];
        if (menu->cnt->down & L_JPAD)
        {
            (*option)--;
            if (*option < menu_option->min)
            {
                *option = menu_option->min;
            }
            menu_write(menu);
        }
        if (menu->cnt->down & R_JPAD)
        {
            (*option)++;
            if (*option > menu_option->max)
            {
                *option = menu_option->max;
            }
            menu_write(menu);
        }
        menu->status = NULL;
        if (menu->option[0] != video_sl || menu->option[1] == video_tb)
        {
            menu->status = "restart to apply changes";
        }
        if (menu->cnt->down & START_BUTTON)
        {
            menu->status = "saving...";
            config_write(menu->config);
            menu->status = NULL;
            menu->update = false;
        }
    }
    else
    {
        if (menu->cnt->down & START_BUTTON)
        {
            menu->update = true;
        }
    }
    return menu->update;
}

static void menu_print(s32 x, s32 y, const char *str)
{
    char c;
    do
    {
        c = *str++;
        if (c == '\t')
        {
            x += 6*4;
        }
        else
        {
            u8 i;
            if (c >= '0' && c <= '9')
            {
                i = c - ('0'-0x00);
            }
            else if (c >= 'A' && c <= 'Z')
            {
                i = c - ('A'-0x0A);
            }
            else if (c >= 'a' && c <= 'z')
            {
                i = c - ('a'-0x0A);
            }
            else switch (c)
            {
                case ':': i = 0x24; break;
                case '(': i = 0x25; break;
                case ')': i = 0x26; break;
                case '-': i = 0x27; break;
                case '?': i = 0x28; break;
                case '!': i = 0x29; break;
                case '+': i = 0x2A; break;
                case '%': i = 0x2B; break;
                case '.': i = 0x2C; break;
                default:  i = 0xFF; break;
            }
            if (i != 0xFF)
            {
                u32 s = 2 + video_sl;
                gSPTextureRectangle(
                    video_gfx++, x << s, y << s, (x+6) << s, (y+7) << s,
                    G_TX_RENDERTILE, 32*6*(i%5), 32*7*(i/5), 0x1000 >> s,
                    0x1000 >> s
                );
            }
            x += 6;
        }
    }
    while (c != 0x00);
}

void menu_draw(struct menu_t *menu)
{
    if (menu->update)
    {
        s32 y;
        u32 i;
        gDPSetCycleType(video_gfx++, G_CYC_FILL);
        gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0x00);
        gDPFillRectangleSL(
            video_gfx++, video_sl,
            (320-6*32)/2, (240-10*(lenof(menu_option_table)+4))/2,
            (320+6*32)/2, (240+10*(lenof(menu_option_table)+4))/2
        );
        gDPSetCycleType(video_gfx++, G_CYC_1CYCLE);
        y = (240-10*(lenof(menu_option_table)+2))/2;
        menu_print((320-6*11)/2, y, "option menu");
        y += 10;
        for (i = 0; i < lenof(menu_option_table); i++)
        {
            const struct menu_option_t *menu_option = &menu_option_table[i];
            char buf[0x20];
            menu_print((320-6*30)/2, y, menu_option->label);
            if (menu_option->table != NULL)
            {
                sprintf(
                    buf, i == menu->index ? "(%s)" : "%s",
                    menu_option->table[menu->option[i]]
                );
            }
            else
            {
                sprintf(buf, i == menu->index ? "(%d)" : "%d", menu->option[i]);
            }
            menu_print((320+6*30)/2 - 6*strlen(buf), y, buf);
            y += 10;
        }
        if (menu->status != NULL)
        {
            menu_print(320/2 - 3*strlen(menu->status), y, menu->status);
        }
        gDPPipeSync(video_gfx++);
    }
}
