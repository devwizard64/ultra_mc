#include <ultra64.h>

#include "main.h"
#include "fault.h"
#include "apphi.h"
#include "video.h"

#define DPC_STATUS      (*(volatile u32 *)0xA410000C)
#define DPC_CLOCK_COUNT (*(volatile u32 *)0xA4100010)
#define DPC_CMD_COUNT   (*(volatile u32 *)0xA4100014)
#define DPC_PIPE_COUNT  (*(volatile u32 *)0xA4100018)
#define DPC_TMEM_COUNT  (*(volatile u32 *)0xA410001C)

#define SCHEDULER_EVENT_VI      0x00
#define SCHEDULER_EVENT_DP      0x01

u64 stack_main[0x400/sizeof(u64)];
static u64 stack_idle[0x800/sizeof(u64)];
static u64 stack_fault[0x800/sizeof(u64)];
static u64 stack_scheduler[0x800/sizeof(u64)];
static u64 stack_apphi[0x2000/sizeof(u64)];
static OSThread thread_idle;
static OSThread thread_fault;
static OSThread thread_scheduler;
static OSThread thread_apphi;

static OSMesgQueue mq_pi;
OSMesgQueue mq_scheduler;
OSMesgQueue mq_apphi_vi;
OSMesgQueue mq_video_vi;
OSMesgQueue mq_video_dp;
static OSMesg msg_pi[4];
static OSMesg msg_scheduler[4];
static OSMesg msg_apphi_vi[1];
static OSMesg msg_video_vi[1];
static OSMesg msg_video_dp[1];

u32 dpc_clock = 0;
u32 dpc_cmd   = 0;
u32 dpc_pipe  = 0;
u32 dpc_tmem  = 0;

static void scheduler_main(unused void *arg)
{
    OSTask *task;
    OSTask *next;
    uint    sync;
    osCreateMesgQueue(MQ(scheduler));
    osCreateMesgQueue(MQ(apphi_vi));
    osCreateMesgQueue(MQ(video_vi));
    osCreateMesgQueue(MQ(video_dp));
    osViSetEvent(&mq_scheduler, (OSMesg)SCHEDULER_EVENT_VI, 1);
    osSetEventMesg(OS_EVENT_DP, &mq_scheduler, (OSMesg)SCHEDULER_EVENT_DP);
    task = next = NULL;
    sync = true;
    while (true)
    {
        OSMesg msg;
        osRecvMesg(&mq_scheduler, &msg, OS_MESG_BLOCK);
        switch ((uintptr_t)msg)
        {
            case SCHEDULER_EVENT_VI:
                if (task == NULL)
                {
                    sync = true;
                }
                osSendMesg(&mq_apphi_vi, 0, OS_MESG_NOBLOCK);
                osSendMesg(&mq_video_vi, 0, OS_MESG_NOBLOCK);
                break;
            case SCHEDULER_EVENT_DP:
                dpc_clock = DPC_CLOCK_COUNT;
                dpc_cmd   = DPC_CMD_COUNT;
                dpc_pipe  = DPC_PIPE_COUNT;
                dpc_tmem  = DPC_TMEM_COUNT;
                task = NULL;
                osSendMesg(&mq_video_dp, 0, OS_MESG_NOBLOCK);
                break;
            default:
                next = msg;
                break;
        }
        if (task == NULL && next != NULL)
        {
            if (!sync)
            {
                continue;
            }
            sync = video_tb;
            DPC_STATUS = 0x0200 | 0x0100 | 0x0080 | 0x0040;
            task = next;
            next = NULL;
            osWritebackDCacheAll();
            osSpTaskStart(task);
        }
    }
}

void vi_init(uint mode)
{
    if (osTvType != OS_TV_NTSC)
    {
        mode += OS_VI_PAL_LPN1 - OS_VI_NTSC_LPN1;
    }
    osViSetMode(&osViModeTable[mode]);
    osViBlack(true);
    osViSetSpecialFeatures(
        OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF |
        OS_VI_DIVOT_ON | OS_VI_DITHER_FILTER_ON
    );
}

static void idle_main(unused void *arg)
{
    osCreateViManager(OS_PRIORITY_VIMGR);
    vi_init(OS_VI_NTSC_LPN1);
    osCreatePiManager(OS_PRIORITY_PIMGR, MQ(pi));
    thread_create(fault,     2, OS_PRIORITY_APPMAX);
    thread_create(scheduler, 3, 0x78);
    thread_create(apphi,     5, 0x20);
    while (true);
}

void main(void)
{
    osInitialize();
    thread_create(idle, 1, OS_PRIORITY_IDLE);
}
