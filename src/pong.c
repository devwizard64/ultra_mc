#include <ultra64.h>

#include "gbi_ext.h"

#include "video.h"
#include "math.h"
#include "printf.h"
#include "pong.h"

#define BALL_S  8
#define BALL_W  BALL_S
#define BALL_H  BALL_S

#define PADDLE_W    16
#define PADDLE_H    48
#define PADDLE_X    72

static const u16 pong_table[] =
{
    U_JPAD, U_JPAD, D_JPAD, D_JPAD,
    L_JPAD, R_JPAD, L_JPAD, R_JPAD,
    B_BUTTON, A_BUTTON, START_BUTTON,
};

void pong_init(struct pong_t *pong, struct controller_t *cnt)
{
    pong->cnt = cnt;
    pong->update = false;
}

static void pong_init_ball(struct pong_t *pong)
{
    pong->score = 0;
    pong->pos[0] = 320.0F;
    pong->pos[1] = 240.0F;
    pong->dir[0] = 0.707106769084930419921875F;
    pong->dir[1] = 0.707106769084930419921875F;
    pong->vel = 1.0F;
}

u8 pong_update(struct pong_t *pong)
{
    u16 held = pong->cnt->held;
    u16 down = pong->cnt->down;
    if (down != 0)
    {
        if ((held & L_TRIG) && down == pong_table[pong->index])
        {
            if (++pong->index == lenof(pong_table))
            {
                pong->index = 0;
                pong->update ^= false^true;
                if (pong->update)
                {
                    pong_init_ball(pong);
                }
            }
        }
        else
        {
            pong->index = 0;
        }
    }
    if (pong->update)
    {
        s32 i;
        pong->paddle = 240 - ((200-PADDLE_H)/64.0F)*pong->cnt->stick[1];
        for (i = 0; i < 8; i++)
        {
            pong->pos[0] += pong->vel*pong->dir[0];
            pong->pos[1] += pong->vel*pong->dir[1];
            if (pong->pos[0] < BALL_W)
            {
                pong_init_ball(pong);
                break;
            }
            if (pong->pos[0] > 600-BALL_W)
            {
                pong->pos[0] = 600-BALL_W;
                pong->dir[0] = -pong->dir[0];
                break;
            }
            if (pong->pos[1] < 40+BALL_H)
            {
                pong->pos[1] = 40+BALL_H;
                pong->dir[1] = -pong->dir[1];
                break;
            }
            if (pong->pos[1] > 440-BALL_H)
            {
                pong->pos[1] = 440-BALL_H;
                pong->dir[1] = -pong->dir[1];
                break;
            }
            if
            (
                pong->pos[1] >= pong->paddle-PADDLE_H &&
                pong->pos[1] <= pong->paddle+PADDLE_H &&
                pong->pos[0] < PADDLE_X+PADDLE_W+BALL_W
            )
            {
                s16 rot;
                pong->pos[0] = PADDLE_X+PADDLE_W+BALL_W;
                rot = ((f32)0x3000/PADDLE_H) * (pong->pos[1]-pong->paddle);
                pong->dir[0] = cos(rot);
                pong->dir[1] = sin(rot);
                if (pong->score < 24)
                {
                    pong->vel += 1.0F/16.0F;
                }
                pong->score++;
                break;
            }
        }
    }
    return pong->update;
}

void pong_draw(struct pong_t *pong)
{
    if (pong->update)
    {
        s32 b_x = pong->pos[0];
        s32 b_y = pong->pos[1];
        s32 p_y = pong->paddle;
        s32 b_ulx = b_x-BALL_W;
        s32 b_uly = b_y-BALL_H;
        s32 b_lrx = b_x+BALL_W;
        s32 b_lry = b_y+BALL_H;
        s32 p_ulx = PADDLE_X;
        s32 p_uly = p_y-PADDLE_H;
        s32 p_lrx = PADDLE_X+PADDLE_W;
        s32 p_lry = p_y+PADDLE_H;
        u32 s = video_sr;
        gDPSetCycleType(video_gfx++, G_CYC_FILL);
        gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0x00);
        gDPFillRectangleSR(video_gfx++, s, b_ulx+2, b_uly+2, b_lrx+2, b_lry+2);
        gDPFillRectangleSR(video_gfx++, s, p_ulx+2, p_uly+2, p_lrx+2, p_lry+2);
        gDPPipeSync(video_gfx++);
        gDPSetFillColorRGB(video_gfx++, 0xFF, 0xFF, 0xFF);
        gDPFillRectangleSR(video_gfx++, s, b_ulx, b_uly, b_lrx, b_lry);
        gDPFillRectangleSR(video_gfx++, s, p_ulx, p_uly, p_lrx, p_lry);
        gDPPipeSync(video_gfx++);
        printf("\nscore:%d\n", pong->score);
    }
}
