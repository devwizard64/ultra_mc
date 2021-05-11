#include <ultra64.h>

#include "gbi_ext.h"

#include "video.h"
#include "chunk.h"
#include "block_gfx.h"

#define VTX_BLOCK_N(x, y, z) \
    {{{BLOCK_W*(x), BLOCK_H*(y), BLOCK_D*(z)}, 0, {0, 0}, {0, 0, 0, 0x00}}}

#define GFX_BLOCK_TEXTURE(n)                                                   \
    static const Gfx gfx_block_texture_##n[] =                                 \
    {                                                                          \
        gsDPLoadTextureBlockN(                                                 \
            G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16*(n), 0,                        \
            G_TX_CLAMP | G_TX_NOMIRROR, G_TX_CLAMP | G_TX_NOMIRROR,            \
            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD                   \
        ),                                                                     \
        gsSPEndDisplayList(),                                                  \
    }

#define UV(u, v) ((u32)(u16)(u) << 16 | (u32)(u16)(v) << 0)
#define GFX_BLOCK_N(n, v0, v1, v2, v3, s0, s1, ul, uh, vl, vh)                 \
    static const Gfx gfx_block_##n##_##ul##uh##vl##vh[] =                      \
    {                                                                          \
        gsSPModifyVertex(v0, G_MWO_POINT_ST, UV(32*16*(ul), 32*16*(vh))),      \
        gsSPModifyVertex(v1, G_MWO_POINT_ST, UV(32*16*(uh), 32*16*(vh))),      \
        gsSPModifyVertex(v2, G_MWO_POINT_ST, UV(32*16*(uh), 32*16*(vl))),      \
        gsSPModifyVertex(v3, G_MWO_POINT_ST, UV(32*16*(ul), 32*16*(vl))),      \
        gsSP2Triangles(v0, v1, v2, s0, v0, v2, v3, s1),                        \
        gsSPEndDisplayList(),                                                  \
    }

#define GFX_BLOCK_XL(ul, uh, vl, vh) \
    GFX_BLOCK_N(xl, 0, 1, 3, 2, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK_XH(ul, uh, vl, vh) \
    GFX_BLOCK_N(xh, 5, 4, 6, 7, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK_YL(ul, uh, vl, vh) \
    GFX_BLOCK_N(yl, 0, 4, 5, 1, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK_YH(ul, uh, vl, vh) \
    GFX_BLOCK_N(yh, 3, 7, 6, 2, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK_ZL(ul, uh, vl, vh) \
    GFX_BLOCK_N(zl, 4, 0, 2, 6, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK_ZH(ul, uh, vl, vh) \
    GFX_BLOCK_N(zh, 1, 5, 7, 3, 0, 0, ul, uh, vl, vh)
#define GFX_BLOCK(ul, uh, vl, vh)                                              \
    GFX_BLOCK_XL(ul, uh, vl, vh);                                              \
    GFX_BLOCK_XH(ul, uh, vl, vh);                                              \
    GFX_BLOCK_YL(ul, uh, vl, vh);                                              \
    GFX_BLOCK_YH(ul, uh, vl, vh);                                              \
    GFX_BLOCK_ZL(ul, uh, vl, vh);                                              \
    GFX_BLOCK_ZH(ul, uh, vl, vh)

#define BLOCK_GFX(xl, xh, yl, yh, zl, zh, mtl)                                 \
{                                                                              \
    gfx_block_xl_##xl, gfx_block_xh_##xh,                                      \
    gfx_block_yl_##yl, gfx_block_yh_##yh,                                      \
    gfx_block_zl_##zl, gfx_block_zh_##zh,                                      \
    mtl                                                                        \
}
#define BLOCK_GFX_N(n, mtl) BLOCK_GFX(n, n, n, n, n, n, mtl)
#define BLOCK_GFX_0(mtl) BLOCK_GFX_N(0101, mtl)
#define BLOCK_GFX_1(mtl) BLOCK_GFX_N(0112, mtl)
#define BLOCK_GFX_2(mtl) BLOCK_GFX_N(0123, mtl)
#define BLOCK_GFX_3(mtl) BLOCK_GFX_N(0134, mtl)
#define BLOCK_GFX_4(mtl) BLOCK_GFX_N(0145, mtl)
#define BLOCK_GFX_5(mtl) BLOCK_GFX_N(0156, mtl)
#define BLOCK_GFX_6(mtl) BLOCK_GFX_N(0167, mtl)
#define BLOCK_GFX_7(mtl) BLOCK_GFX_N(0178, mtl)

static const u8 texture_block_outline[] =
{
    #include "build/src/block_outline.ia4.h"
};

static const u16 texture_block_00[] =
{
#include "build/src/block/bedrock.rgba16.h"
#include "build/src/block/stone.rgba16.h"
#include "build/src/block/cobblestone.rgba16.h"
#include "build/src/block/dirt.rgba16.h"
#include "build/src/block/grass_side.rgba16.h"
#include "build/src/block/grass_top.rgba16.h"
};

const Vtx vtx_chunk[] =
{
    VTX_BLOCK_N(CHUNK_W*0, CHUNK_H*0, CHUNK_D*0),
    VTX_BLOCK_N(CHUNK_W*1, CHUNK_H*0, CHUNK_D*0),
    VTX_BLOCK_N(CHUNK_W*0, CHUNK_H*1, CHUNK_D*0),
    VTX_BLOCK_N(CHUNK_W*1, CHUNK_H*1, CHUNK_D*0),
    VTX_BLOCK_N(CHUNK_W*0, CHUNK_H*0, CHUNK_D*1),
    VTX_BLOCK_N(CHUNK_W*1, CHUNK_H*0, CHUNK_D*1),
    VTX_BLOCK_N(CHUNK_W*0, CHUNK_H*1, CHUNK_D*1),
    VTX_BLOCK_N(CHUNK_W*1, CHUNK_H*1, CHUNK_D*1),
};

static const Vtx vtx_block_outline[] =
{
    VTX_BLOCK_N(0, 0, 0),
    VTX_BLOCK_N(1, 0, 0),
    VTX_BLOCK_N(0, 1, 0),
    VTX_BLOCK_N(1, 1, 0),
    VTX_BLOCK_N(0, 0, 1),
    VTX_BLOCK_N(1, 0, 1),
    VTX_BLOCK_N(0, 1, 1),
    VTX_BLOCK_N(1, 1, 1),
};

const Gfx gfx_block_outline[] =
{
    gsDPPipeSync(),
    gsSPDisplayList(gfx_rm_zb_xlu_decal),
    gsDPLoadTextureBlock_4b(
        texture_block_outline, G_IM_FMT_IA, 16, 16, 0, G_TX_WRAP | G_TX_MIRROR,
        G_TX_WRAP | G_TX_MIRROR, 4, 4, G_TX_NOLOD, G_TX_NOLOD
    ),
    gsSPVertex(vtx_block_outline,  8,  0),
    gsSPModifyVertex( 0, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 4, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 6, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 2, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSPModifyVertex( 5, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 1, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 3, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 7, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSP2Triangles( 0,  4,  6,  0,  0,  6,  2,  0),
    gsSP2Triangles( 5,  1,  3,  0,  5,  3,  7,  0),
    gsSPModifyVertex( 0, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 1, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 5, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 4, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSPModifyVertex( 6, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 7, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 3, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 2, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSP2Triangles( 0,  1,  5,  0,  0,  5,  4,  0),
    gsSP2Triangles( 6,  7,  3,  0,  6,  3,  2,  0),
    gsSPModifyVertex( 1, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 0, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 2, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 3, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSPModifyVertex( 4, G_MWO_POINT_ST, UV(32*-16, 32*-16)),
    gsSPModifyVertex( 5, G_MWO_POINT_ST, UV(32* 16, 32*-16)),
    gsSPModifyVertex( 7, G_MWO_POINT_ST, UV(32* 16, 32* 16)),
    gsSPModifyVertex( 6, G_MWO_POINT_ST, UV(32*-16, 32* 16)),
    gsSP2Triangles( 1,  0,  2,  0,  1,  2,  3,  0),
    gsSP2Triangles( 4,  5,  7,  0,  4,  7,  6,  0),
    gsSPPopMatrix(G_MTX_MODELVIEW),
    gsSPEndDisplayList(),
};

static const Gfx gfx_block_end_start[] =
{
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCombineLERP(
        1, TEXEL0, TEXEL0_ALPHA, 0, 0, 0, 0, 1,
        1, TEXEL0, TEXEL0_ALPHA, 0, 0, 0, 0, 1
    ),
    gsSPEndDisplayList(),
};

static const Gfx gfx_block_end_end[] =
{
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsSPEndDisplayList(),
};

const Gfx gfx_block_end_lo[] =
{
    gsSPDisplayList(gfx_block_end_start),
    gsDPLoadTextureTileN(
        G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 240,
        320/2-5,
        240/2-5,
        320/2+4-1,
        240/2+4-1,
        0,
        G_TX_CLAMP | G_TX_NOMIRROR,
        G_TX_CLAMP | G_TX_NOMIRROR,
        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD
    ),
    gsSPTextureRectangle(
        (320/2-5) << 2, (240/2-1) << 2, (320/2+4) << 2, (240/2+0) << 2,
        G_TX_RENDERTILE,
        (320/2-5) << 5, (240/2-1) << 5, 0x0400, 0x0400
    ),
    gsSPTextureRectangle(
        (320/2-1) << 2, (240/2-5) << 2, (320/2+0) << 2, (240/2+4) << 2,
        G_TX_RENDERTILE,
        (320/2-1) << 5, (240/2-5) << 5, 0x0400, 0x0400
    ),
    gsSPBranchList(gfx_block_end_end),
};

const Gfx gfx_block_end_hi[] =
{
    gsSPDisplayList(gfx_block_end_start),
    gsDPLoadTextureTileN(
        G_IM_FMT_RGBA, G_IM_SIZ_16b, 640, 480,
        640/2-10,
        480/2-10,
        640/2+8-1,
        480/2+8-1,
        0,
        G_TX_CLAMP | G_TX_NOMIRROR,
        G_TX_CLAMP | G_TX_NOMIRROR,
        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD
    ),
    gsSPTextureRectangle(
        (640/2-10) << 2, (480/2-2) << 2, (640/2+8) << 2, (480/2+0) << 2,
        G_TX_RENDERTILE,
        (640/2-10) << 5, (480/2-2) << 5, 0x0400, 0x0400
    ),
    gsSPTextureRectangle(
        (640/2-2) << 2, (480/2-10) << 2, (640/2+0) << 2, (480/2+8) << 2,
        G_TX_RENDERTILE,
        (640/2-2) << 5, (480/2-10) << 5, 0x0400, 0x0400
    ),
    gsSPBranchList(gfx_block_end_end),
};

GFX_BLOCK_TEXTURE(6);

static const Gfx gfx_block_mtl_00[] =
{
    gsDPPipeSync(),
    gsSPLoadGeometryMode(G_ZBUFFER | G_CULL_BACK),
    gsSPTexture(0x8000, 0x8000, G_TX_NOLOD, G_TX_RENDERTILE, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsSPDisplayList(gfx_rm_zb_opa_surf),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_block_00),
    gsSPBranchList(gfx_block_texture_6),
};

GFX_BLOCK(0, 1, 0, 1);
GFX_BLOCK(0, 1, 1, 2);
GFX_BLOCK(0, 1, 2, 3);
GFX_BLOCK(0, 1, 3, 4);
GFX_BLOCK_XL(0, 1, 4, 5);
GFX_BLOCK_XH(0, 1, 4, 5);
GFX_BLOCK_ZL(0, 1, 4, 5);
GFX_BLOCK_ZH(0, 1, 4, 5);
GFX_BLOCK_YH(0, 1, 5, 6);

const Gfx *const block_mtl_table[BLOCK_MTL_LEN] =
{
    gfx_block_mtl_00,
};

const struct block_gfx_t block_gfx_table[BLOCK_LEN] =
{
    /* NULL        */ {NULL, NULL, NULL, NULL, NULL, NULL, 0x00},
    /* BEDROCK     */ BLOCK_GFX_0(0x00),
    /* STONE       */ BLOCK_GFX_1(0x00),
    /* COBBLESTONE */ BLOCK_GFX_2(0x00),
    /* DIRT        */ BLOCK_GFX_3(0x00),
    /* GRASS       */ BLOCK_GFX(0145, 0145, 0134, 0156, 0145, 0145, 0x00),
};
