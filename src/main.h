#ifndef _MAIN_H_
#define _MAIN_H_

#include <ultra64.h>
#include <types.h>

#define MQ(m) &mq_##m, msg_##m, lenof(msg_##m)

#define thread_create(t, id, pri)           \
{                                           \
    osCreateThread(                         \
        &thread_##t, id, t##_main, NULL,    \
        stack_##t+lenof(stack_##t), pri     \
    );                                      \
    osStartThread(&thread_##t);             \
}

extern OSMesgQueue mq_scheduler;
extern OSMesgQueue mq_apphi_vi;
extern OSMesgQueue mq_video_vi;
extern OSMesgQueue mq_video_dp;

extern u32 dpc_clock;
extern u32 dpc_cmd;
extern u32 dpc_pipe;
extern u32 dpc_tmem;

extern void vi_init(uint mode);

#endif /* _MAIN_H_ */
