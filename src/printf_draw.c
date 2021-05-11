#include <ultra64.h>

#include "video.h"
#include "printf.h"

char *printf_str = printf_buf;

void printf_draw(void)
{
    char *str = printf_str = printf_buf;
    if (*str != 0x00)
    {
        s32 y;
        s32 x;
        char c;
        gDPSetCycleType(video_gfx++, G_CYC_1CYCLE);
        y = 20;
        x = 20;
        do
        {
            c = *str++;
            if (c == '\t')
            {
                x += 6*4;
            }
            else if (c == '\n')
            {
                x = 20;
                y += 8;
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
            if (x >= 320-20)
            {
                x = 20;
                y += 8;
            }
        }
        while (y < 240-20-7 && c != 0x00);
        gDPPipeSync(video_gfx++);
    }
}
