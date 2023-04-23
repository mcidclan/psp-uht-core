// m-c/d
#pragma once

#include <pspkernel.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <psputility_usbmodules.h>
#include <psputility_avmodules.h>
#include <pspusb.h>
#include <pspusbacc.h>
#include <pspusbcam.h>
#include <pspjpeg.h>

typedef struct vec2i{
  int x, y;
} vec2i;
typedef struct vec4i{
  int x, y, z, w;
} vec4i;

void getAbsPosition(const vec4i in, vec2i *out);
void getOrtPosition(const vec4i in, vec2i *out);
