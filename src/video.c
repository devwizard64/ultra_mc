#include <ultra64.h>

#include "gbi_ext.h"

#include "main.h"
#include "apphi.h"
#include "video.h"
#include "mem.h"
#include "math.h"
#include "profiler.h"
#include "printf.h"
#include "pong.h"
#include "player.h"
#include "world.h"

static const u8 texture_font[] =
{
#include "build/src/font.i4.h"
};

static const Gfx gfx_font_start[] =
{
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCombineLERP(
        0, 0, 0, TEXEL0, 0, 0, 0, 1,
        0, 0, 0, TEXEL0, 0, 0, 0, 1
    ),
    gsDPLoadTextureBlock_4b(
        texture_font, G_IM_FMT_I, 32, 64, 0, G_TX_CLAMP | G_TX_NOMIRROR,
        G_TX_CLAMP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
        G_TX_NOLOD
    ),
    gsSPEndDisplayList(),
};

static const u32 video_rm_table[][4] =
{
    {
        G_RM_ZB_OPA_SURF    | G_RM_ZB_OPA_SURF2,
        G_RM_RA_ZB_OPA_SURF | G_RM_RA_ZB_OPA_SURF2,
        G_RM_RA_ZB_OPA_SURF | G_RM_RA_ZB_OPA_SURF2,
        G_RM_AA_ZB_OPA_SURF | G_RM_AA_ZB_OPA_SURF2,
    },
    {
        G_RM_ZB_XLU_DECAL    | G_RM_ZB_XLU_DECAL2,
        G_RM_ZB_XLU_DECAL    | G_RM_ZB_XLU_DECAL2,
        G_RM_AA_ZB_XLU_DECAL | G_RM_AA_ZB_XLU_DECAL2,
        G_RM_AA_ZB_XLU_DECAL | G_RM_AA_ZB_XLU_DECAL2,
    },
};

Gfx gfx_video_rm[] =
{
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsSPEndDisplayList(),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsSPEndDisplayList(),
};

static u64 video_task_stack[SP_DRAM_STACK_SIZE8/sizeof(u64)];
static u64 video_task_output[0x4000/sizeof(u64)];
static Gfx video_buf_table[2][0x2000];
OSTask video_task_table[2];

static OSMesgQueue mq_video_prenmi;
static OSMesg msg_video_prenmi[1];

u16    *video_cimg[3];
u16    *video_zimg;
f32     video_aspect;
u8      video_sl;
u8      video_sr;
u8      video_tb;
OSTime  video_time_table[4];
OSTask *video_task;
Gfx    *video_buf;
Gfx    *video_gfx;
u8     *video_mem;

u32 video_frame   = 0;
u8  video_cimg_vi = 0;
u8  video_cimg_dp = 1;

void video_init(void)
{
    u32 i;
    s32 w;
    s32 h;
    size_t size;
    size_t csiz;
    osCreateMesgQueue(MQ(video_prenmi));
    osSetEventMesg(OS_EVENT_PRENMI, &mq_video_prenmi, (OSMesg)0);
    for (i = 0; i < lenof(video_task_table); i++)
    {
        OSTask *task = &video_task_table[i];
        task->t.type             = M_GFXTASK;
        task->t.flags            = 0;
        task->t.ucode_boot       = rspbootTextStart;
        task->t.ucode_boot_size  = (u8 *)rspbootTextEnd-(u8 *)rspbootTextStart;
        task->t.ucode            = gspF3DEX2_fifoTextStart;
        task->t.ucode_size       = SP_UCODE_SIZE;
        task->t.ucode_data       = gspF3DEX2_fifoDataStart;
        task->t.ucode_data_size  = SP_UCODE_DATA_SIZE;
        task->t.dram_stack       = video_task_stack;
        task->t.dram_stack_size  = sizeof(video_task_stack);
        task->t.output_buff      = video_task_output;
        task->t.output_buff_size = video_task_output + lenof(video_task_output);
        task->t.yield_data_ptr   = NULL;
        task->t.yield_data_size  = 0;
    }
    w = 320 << video_sl;
    h = 240 << video_sl;
    size = sizeof(u16)*w*h;
    csiz = 2*size;
    if (video_tb)
    {
        csiz += size;
    }
    video_cimg[0] = mem_alloc_r(csiz);
    video_cimg[1] = (u16 *)((u8 *)video_cimg[0] + size);
    video_cimg[2] = (u16 *)((u8 *)video_cimg[1] + size);
    video_zimg = (u16 *)(((u32)mem_alloc_l(size+0x40)+0x3F) & ~0x3F);
    vi_init(video_sl == 0 ? OS_VI_NTSC_LAN1 : OS_VI_NTSC_HAF1);
    osSendMesg(&mq_video_dp, (OSMesg)0, OS_MESG_NOBLOCK);
}

static void video_start(void)
{
    OSTime a = video_time_table[video_frame & 3];
    OSTime b = video_time_table[video_frame & 3] = osGetTime();
    Vp *vp;
    s32 w;
    s32 h;
    s32 x;
    s32 y;
    u16 cn;
    u16 cp;
    u32 i;
    printf(
        "fps:%.2f  gfx:%4d  ",
        0.005F + 4.0F*osClockRate/(b-a),
        video_mem - (u8 *)video_gfx
    );
    video_task = &video_task_table[video_frame & 1];
    video_buf  = video_buf_table[video_frame & 1];
    video_gfx  = video_buf;
    video_mem  = (u8 *)video_buf + sizeof(video_buf_table[0]);
    video_frame++;

    vp = mem_alloc_gfx(sizeof(*vp));
    w = 320 << video_sl;
    h = 240 << video_sl;
    x = app_config.v_x;
    y = app_config.v_y;
    vp->vp.vscale[0] = 2*w - 4*x;
    vp->vp.vscale[1] = 2*h - 4*y;
    vp->vp.vscale[2] = G_MAXZ;
    vp->vp.vscale[3] = 0;
    vp->vp.vtrans[0] = 2*w;
    vp->vp.vtrans[1] = 2*h;
    vp->vp.vtrans[2] = 0;
    vp->vp.vtrans[3] = 0;
    video_aspect = (f32)vp->vp.vscale[0] / (f32)vp->vp.vscale[1];
    cn = 1+app_config.v_clip;
    cp = -cn;
    gDPSetOtherMode(
        video_gfx++,
        G_AD_DISABLE |
        G_CD_MAGICSQ |
        G_CK_NONE |
        G_TC_FILT |
        G_TF_POINT |
        G_TT_NONE |
        G_TL_TILE |
        G_TD_CLAMP |
        G_TP_PERSP |
        G_CYC_FILL |
        (app_config.v_pm ? G_PM_1PRIMITIVE : G_PM_NPRIMITIVE),
        G_AC_NONE |
        G_ZS_PIXEL |
        G_RM_NOOP | G_RM_NOOP2
    );
    gDPSetScissorFrac(
        video_gfx++, G_SC_NON_INTERLACE, 0 << 2, 0 << 2, w << 2, h << 2
    );
    gDPSetDepthImage(video_gfx++, video_zimg);
    gDPSetColorImage(video_gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, video_zimg);
    gDPSetFillColor(video_gfx++, 0x00010001U*GPACK_ZDZ(G_MAXFBZ, 0));
    gDPFillRectangle(video_gfx++, x, y, w-x-1, h-y-1);
    gDPPipeSync(video_gfx++);
    gDPSetColorImage(
        video_gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, video_cimg[video_cimg_dp]
    );
    gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0x00);
    gDPFillRectangle(video_gfx++, 0, 0, w-1, h-1);
    gDPPipeSync(video_gfx++);
    gDPSetScissorFrac(
        video_gfx++, G_SC_NON_INTERLACE, x << 2, y << 2, (w-x) << 2, (h-y) << 2
    );
    gDPSetCycleType(video_gfx++, G_CYC_1CYCLE);
    gSPViewport(video_gfx++, vp);
    gMoveWd(video_gfx++, G_MW_CLIP, G_MWO_CLIP_RNX, cn);
    gMoveWd(video_gfx++, G_MW_CLIP, G_MWO_CLIP_RNY, cn);
    gMoveWd(video_gfx++, G_MW_CLIP, G_MWO_CLIP_RPX, cp);
    gMoveWd(video_gfx++, G_MW_CLIP, G_MWO_CLIP_RPY, cp);
    for (i = 0; i < lenof(video_rm_table); i++)
    {
        gfx_video_rm[2*i].words.w1 = video_rm_table[i][app_config.v_aa];
    }
}

static void video_end(void)
{
    gDPFullSync(video_gfx++);
    gSPEndDisplayList(video_gfx++);
    osRecvMesg(&mq_video_dp, NULL, OS_MESG_BLOCK);
    video_draw(video_task, video_buf, (u8 *)video_gfx - (u8 *)video_buf);
    mem_update_gfx();
    osViSwapBuffer(video_cimg[video_cimg_vi]);
    osViBlack(false);
    if (++video_cimg_vi >= 2+video_tb)
    {
        video_cimg_vi = 0;
    }
    if (++video_cimg_dp >= 2+video_tb)
    {
        video_cimg_dp = 0;
    }
    osRecvMesg(&mq_video_vi, NULL, OS_MESG_BLOCK);
}

static void video_update(void)
{
    video_start();
    mem_draw();
    player_draw(&app_player);
    world_draw(&app_world);
    gDPSetScissorFrac(
        video_gfx++, G_SC_NON_INTERLACE,
        0 << 2,
        0 << 2,
        (320 << 2) << video_sl,
        (240 << 2) << video_sl
    );
    gSPDisplayList(video_gfx++, gfx_font_start);
    pong_draw(&app_pong);
    menu_draw(&app_menu);
    printf_draw();
    profiler_draw();
    video_end();
}

static void video_update_reset(void)
{
    u16 *cimg = video_cimg[video_cimg_vi];
    s32 i;
    for (i = 0; i < 8; i++)
    {
        u32 *dst = (u32 *)cimg;
        u32 *src = (u32 *)cimg;
        u32 w = 320 << video_sl;
        u32 x = 20 << video_sl;
        u32 y = 15 << video_sl;
        dst += 16/2 * (w*(rng_u16()%y)+(rng_u16()%x));
        src += 16/2 * (w*(rng_u16()%y)+(rng_u16()%x));
        for (y = 0; y < 16; y++)
        {
            for (x = 0; x < 16/2; x++)
            {
                u32 d = *dst;
                *dst = *src;
                *src = d;
                dst++;
                src++;
            }
            dst += (w-16) / 2;
            src += (w-16) / 2;
        }
    }
    osWritebackDCacheAll();
    osRecvMesg(&mq_video_vi, NULL, OS_MESG_BLOCK);
    osViSwapBuffer(cimg);
}

void video_main(unused void *arg)
{
    do
    {
        video_update();
    }
    while (mq_video_prenmi.validCount == 0);
    do
    {
        video_update_reset();
    }
    while (true);
}
