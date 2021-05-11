#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <ultra64.h>
#include <types.h>

#define VIDEO_MODE_LO           0x00
#define VIDEO_MODE_HI           0x01
#define VIDEO_AA_NONE           0x00
#define VIDEO_AA_MIN            0x01
#define VIDEO_AA_REDUCED        0x02
#define VIDEO_AA_FULL           0x03

#define gfx_rm_zb_opa_surf      &gfx_video_rm[0x00]
#define gfx_rm_zb_xlu_decal     &gfx_video_rm[0x02]

#define video_draw(task, gfx, size)                     \
{                                                       \
    OSTask    *_task = task;                            \
    const Gfx *_gfx  = gfx;                             \
    size_t     _size = size;                            \
    _task->t.data_ptr  = (u64 *)_gfx;                   \
    _task->t.data_size = _size;                         \
    osSendMesg(&mq_scheduler, _task, OS_MESG_BLOCK);    \
}

extern OSTask video_task_table[2];

extern Gfx gfx_video_rm[];

extern u16 *video_cimg[3];
extern u16 *video_zimg;
extern u32 video_frame;
extern u8  video_cimg_vi;
extern u8  video_cimg_dp;

extern OSTime  video_time_table[4];
extern f32     video_aspect;
extern u8      video_sl;
extern u8      video_sr;
extern u8      video_tb;
extern OSTask *video_task;
extern Gfx    *video_buf;
extern Gfx    *video_gfx;
extern u8     *video_mem;

extern void video_init(void);
extern void video_main(void *);

#endif /* _VIDEO_H_ */
