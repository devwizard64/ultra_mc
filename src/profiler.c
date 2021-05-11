#include <ultra64.h>

#include "gbi_ext.h"

#include "main.h"
#include "video.h"
#include "mem.h"
#include "printf.h"

extern u8 bss_main_end[];

void profiler_draw(void)
{
    u32 s;
    u32 w_dpp;
    u32 w_dpc;
    u32 w_prg;
    struct heap_t *heap;
    gDPSetCycleType(video_gfx++, G_CYC_FILL);
    gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0x00);
#define W 512
#define H 8
#define X ((640-W)/2)
#define Y (480-40 - H-2-H)
    s = video_sr;
    gDPFillRectangleSR(video_gfx++, s, X-2, Y-2, X+W+2, Y+H+2+H+2);
    gDPPipeSync(video_gfx++);
    if (dpc_clock != 0)
    {
        w_dpp = W * dpc_pipe / dpc_clock;
        w_dpc = W * dpc_cmd  / dpc_clock;
    }
    else
    {
        w_dpp = 0;
        w_dpc = 0;
    }
    gDPSetFillColorRGB(video_gfx++, 0x00, 0xFF, 0x00);
    gDPFillRectangleSR(video_gfx++, s, X, Y, X+w_dpp, Y+H);
    gDPPipeSync(video_gfx++);
    gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0xFF);
    gDPFillRectangleSR(video_gfx++, s, X, Y, X+w_dpc, Y+H);
    gDPPipeSync(video_gfx++);
#undef Y
#define Y (480-40 - H)
    w_prg = OS_K0_TO_PHYSICAL(bss_main_end) / 0x4000;
    gDPSetFillColorRGB(video_gfx++, 0x00, 0x00, 0xFF);
    gDPFillRectangleSR(video_gfx++, s, X, Y, X+w_prg, Y+H);
    gDPPipeSync(video_gfx++);
    if (osMemSize < 0x00800000)
    {
        gDPSetFillColorRGB(video_gfx++, 0xFF, 0x00, 0x00);
        gDPFillRectangleSR(video_gfx++, s, 640/2, Y, (640+W)/2, Y+H);
        gDPPipeSync(video_gfx++);
    }
    gDPSetFillColorRGB(video_gfx++, 0xFF, 0xFF, 0xFF);
    heap = mem_heap;
    while (heap != NULL)
    {
        if (heap->sig == HEAP_SIG)
        {
            u32 xl = OS_K0_TO_PHYSICAL(heap+0);
            u32 xh = OS_K0_TO_PHYSICAL(heap+1);
            if (!heap->free)
            {
                xh += heap->size;
            }
            xl /= 0x4000;
            xh /= 0x4000;
            gDPFillRectangleSR(video_gfx++, s, X+xl, Y, X+xh, Y+H);
        }
        heap = heap->next;
    }
    gDPPipeSync(video_gfx++);
}
