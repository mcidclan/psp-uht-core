// m-c/d
#pragma once

#include "uht_util.h"
#include <psppower.h>
#include <psputility_usbmodules.h>
#include <psputility_avmodules.h>
#include <pspusb.h>
#include <pspusbacc.h>
#include <pspusbcam.h>
#include <pspjpeg.h>

#define WW 160
#define HH 120
#define MAX_VIDEO_FRAME_SIZE (1024*34)

static const u32 SIZE_FBu8  = WW*HH;
static const u32 SIZE_FBu16 = SIZE_FBu8*2;
static const u32 SIZE_FBu32 = SIZE_FBu16*2;

class Uht{
  public:
  vec4i pOut;
  u32 *FB_MPEG;
  u32 *FB_8888;
  u16 *FB_565;
  u8 *B_WORK;
  bool run;
  bool direction;
  bool checkAmbientLight; 
  bool relativeClean;
  int thid;
  static Uht * myUhtPt;
  static int video_thread(SceSize args, void *argp);
  Uht();
  int initCam(const u8 prop = 0x10);
  void setGrid(const u8 gSize, vec3i *grids);
  void initBuffer(void *bWork, const bool dRender, const bool fb565);
  void setYcbcrThresh(u8 threshNum);
  void setMinDim(const u8 xMin, const u8 yMin);
  void setMaxMrg(const u8 xMrg, const u8 yMrg);
  void fm_activated();
  bool fm_isWaiting();
  void fm_nextFrame();
  ~Uht();
  private:
  int semaid;
  u8 gSize;
  vec3i *grids;
  u32 margRB;
  u8 threshNum;
  u8 xMin, yMin;
  u8 xMrg, yMrg;
  bool fb565;
  bool semaState;
  bool appSemaState;
  bool wFSignalSema;
  u8 *xCont;
  u8 *yCont;
  u8  work[1024*68] __attribute__((aligned(64)));
  u8  buffer[MAX_VIDEO_FRAME_SIZE] __attribute__((aligned(64)));
  void relativeCleaner(bool absClean = false);
  int internalThread(SceSize args, void *argp);
  void convertBto565();
  int InitJpegDecoder();
  void LoadModules();
  void StartUsb();
  int FinishJpegDecoder();
  void UnloadModules();
  void StopUsb();
};

