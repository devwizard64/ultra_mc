#ifndef _GBI_EXT_H_
#define _GBI_EXT_H_

#include <ultra64.h>
#include <types.h>

#define gsSPSetLights1N(name)                                                  \
    gsSPLight(&name.l[0],1),                                                   \
    gsSPLight(&name.a,2)

#define gDPSetFillColorRGB(pkt, r, g, b)                                       \
    gDPSetFillColor(pkt, 0x00010001U*GPACK_RGBA5551(r, g, b, 1))
#define gsDPSetFillColorRGB(r, g, b)                                           \
    gsDPSetFillColor(0x00010001U*GPACK_RGBA5551(r, g, b, 1))

#define gsDPLoadTextureBlockN(                                                 \
    fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt       \
)                                                                              \
    gsDPSetTile(                                                               \
        fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt,     \
        cms, masks, shifts                                                     \
    ),                                                                         \
    gsDPLoadSync(),                                                            \
    gsDPLoadBlock(                                                             \
        G_TX_LOADTILE, 0, 0,                                                   \
        (((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,                    \
        CALC_DXT(width, siz##_BYTES)                                           \
    ),                                                                         \
    gsDPPipeSync(),                                                            \
    gsDPSetTile(                                                               \
        fmt, siz, ((((width) * siz##_LINE_BYTES)+7)>>3), 0, G_TX_RENDERTILE,   \
        pal, cmt, maskt, shiftt, cms, masks, shifts                            \
    ),                                                                         \
    gsDPSetTileSize(                                                           \
        G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC,            \
        ((height)-1) << G_TEXTURE_IMAGE_FRAC                                   \
    )

#define gsDPLoadTextureTileN(                                                  \
    fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt,  \
    shifts, shiftt                                                             \
)                                                                              \
    gsDPSetTile(                                                               \
        fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0,            \
        G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts               \
    ),                                                                         \
    gsDPLoadSync(),                                                            \
    gsDPLoadTile(                                                              \
        G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC,                            \
        (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC,              \
        (lrt)<<G_TEXTURE_IMAGE_FRAC                                            \
    ),                                                                         \
    gsDPPipeSync(),                                                            \
    gsDPSetTile(                                                               \
        fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0,            \
        G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts           \
    ),                                                                         \
    gsDPSetTileSize(                                                           \
        G_TX_RENDERTILE, (uls)<<G_TEXTURE_IMAGE_FRAC,                          \
        (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC,              \
        (lrt)<<G_TEXTURE_IMAGE_FRAC                                            \
    )

#define gsDPLoadTextureBlock_4bN(                                              \
    fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt            \
)                                                                              \
    gsDPSetTile(                                                               \
        fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms,    \
        masks, shifts                                                          \
    ),                                                                         \
    gsDPLoadSync(),                                                            \
    gsDPLoadBlock(                                                             \
        G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)   \
    ),                                                                         \
    gsDPPipeSync(),                                                            \
    gsDPSetTile(                                                               \
        fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, \
        maskt, shiftt, cms, masks, shifts                                      \
    ),                                                                         \
    gsDPSetTileSize(                                                           \
        G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC,            \
        ((height)-1) << G_TEXTURE_IMAGE_FRAC                                   \
    )

#define gDPFillRectangleSL(pkt, s, ulx, uly, lrx, lry)                         \
    gDPFillRectangle(                                                          \
        pkt, (ulx) << (s), (uly) << (s), ((lrx) << (s))-1, ((lry) << (s))-1    \
    )
#define gsDPFillRectangleSL(s, ulx, uly, lrx, lry)                             \
    gsDPFillRectangle(                                                         \
        (ulx) << (s), (uly) << (s), ((lrx) << (s))-1, ((lry) << (s))-1         \
    )

#define gDPFillRectangleSR(pkt, s, ulx, uly, lrx, lry)                         \
    gDPFillRectangle(                                                          \
        pkt, (ulx) >> (s), (uly) >> (s), ((lrx) >> (s))-1, ((lry) >> (s))-1    \
    )

#endif /* _GBI_EXT_H_ */
