// m-c/d
#pragma once

#include <pspkernel.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define YCBCR_RESTRICTIVE_THRESH 0
#define YCBCR_APPROPRIATE_THRESH 1
#define YCBCR_PERMISSIVE_THRESH 2

#define YCBCR_BEL06_THRESH 3

typedef struct vec2i{
  int x, y;
} vec2i;

typedef struct vec3i{
  int x, y, z;
} vec3i;

typedef struct vec4i{
  int x, y, z, w;
} vec4i;


void medianFilter(const u32* in, u32* out, const u8 xend, const u8 yend);
void YCbCr(const u32* in, u8* out, const u16 size, const u8 rInt, const u8 tn);
void gridEval(u8 *in, const u8 cXsize, const u8 cYsize, const u16 pThresh);
void getMargRB(const u32 *in, u32 &margRB);
void getQtInfo(const u8* in, u8 *xcont, u8 *ycont, const u8 xend, const u8 yend);

vec4i getBeInfo(const u8* in, u8 *xcont, u8 *ycont,
    const u8 xend, const u8 yend,
    const u8 xMin, const u8 yMin);

vec4i getFeInfo(const u8* in, u8 *xcont, u8 *ycont,
    const u8 xend, const u8 yend,
    const u8 xMin, const u8 yMin,
    const u8 xMrg, const u8 yMrg);

void getAbsPosition(const vec4i in, vec2i *out);
void getOrtPosition(const vec4i in, vec2i *out);
void getSqrDimension(const vec4i sqr, vec2i *out);
float evalStdHand(const vec4i sqr);


