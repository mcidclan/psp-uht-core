// m-c/d
#include "uht.h"

Uht *Uht::myUhtPt;

Uht::Uht(){
  thid = -1;
  myUhtPt = this;
  xMin = 24;
  yMin = 24;
  xMrg = 255;
  yMrg = 255;
  gSize = 0;
  margRB = 0;
  threshNum = 0;
  checkAmbientLight = false;
  run = false;
  direction = false;
  wFSignalSema = false;
  relativeClean = false;
}

void  Uht::fm_activated(){
  this->wFSignalSema = true;
  semaid = sceKernelCreateSema("OWTSema", 0, 2, 2, 0);
  appSemaState = false;
  semaState = false;
}

bool Uht::fm_isWaiting(){
  if(semaState && !appSemaState && run){
    appSemaState = true;
    return true;
  }
  return false;
}

void Uht::fm_nextFrame(){
  sceKernelSignalSema(semaid, 1);
}

void Uht::setYcbcrThresh(u8 threshNum){
  this->threshNum = threshNum;
}

void Uht::setGrid(const u8 gSize, vec3i *grids){
  this->grids = grids;
  this->gSize = gSize;
}

void Uht::setMinDim(const u8 xMin, const u8 yMin){
  this->xMin = xMin;
  this->yMin = yMin;
}

void Uht::setMaxMrg(const u8 xMrg, const u8 yMrg){
  this->xMrg = xMrg;
  this->yMrg = yMrg;  
}

void Uht::initBuffer(void *bWork, const bool dRender, const bool fb565){
  this->fb565 = fb565;
  u32 ptValue = (u32)bWork;
  xCont       = (u8*)  (ptValue);
  yCont       = (u8*)  (ptValue + 0x000C0);
  FB_565      = (u16*)(ptValue + 0x00140);
  B_WORK      = (u8*)  (ptValue + 0x09740);
  FB_8888     = (u32*)(ptValue + 0x0E240);
  FB_MPEG     = dRender ? FB_8888 : (u32*)(ptValue + 0x20E40);
}

int Uht::initCam(const u8 prop){
  LoadModules();
  StartUsb();
  if (sceUsbActivate(PSP_USBCAM_PID) < 0) return 0;
  if (InitJpegDecoder() < 0) return 0;
  thid = sceKernelCreateThread("video_thread", Uht::video_thread, prop, 0x400*32, 0, NULL);
  if (thid < 0) return 0;
  if (sceKernelStartThread(thid, 0, NULL) < 0) return 0;
  return 1;
}

int Uht::video_thread(SceSize args, void *argp){
  myUhtPt->internalThread(args, argp);
  return 1;
}

int Uht::internalThread(SceSize args, void *argp) {
  u8 i = 0;
  PspUsbCamSetupVideoParam videoparam;
  memset(&videoparam, 0, sizeof(videoparam));
  videoparam.size = sizeof(videoparam);
  videoparam.resolution = PSP_USBCAM_RESOLUTION_160_120;
  videoparam.framerate = PSP_USBCAM_FRAMERATE_30_FPS;
  videoparam.saturation = 252;
  videoparam.brightness = 138;
  videoparam.contrast = 62;
  videoparam.framesize = MAX_VIDEO_FRAME_SIZE;
  videoparam.unk = 1;
  videoparam.evlevel = PSP_USBCAM_EVLEVEL_0_0;
  int result = 0;
  while(1){
    sceKernelDelayThread(10);
    while ((sceUsbGetState() & 0xF) != PSP_USB_CONNECTION_ESTABLISHED){
      sceKernelDelayThread(1000);
    }
    result = sceUsbCamSetupVideo(&videoparam, work, sizeof(work));
    if (result < 0) goto out_2;
    sceUsbCamAutoImageReverseSW(1);
    result = sceUsbCamStartVideo();
    if (result < 0) goto out_2;
    while(1){
      u8 outOfUsb = 0;
      direction = sceUsbCamGetLensDirection();
      while((result = sceUsbCamReadVideoFrameBlocking(buffer, MAX_VIDEO_FRAME_SIZE)) < 0){
        outOfUsb += 1;
        if(outOfUsb > 10) goto out_1;
      };
      sceKernelDcacheWritebackAll();
      result = sceJpegDecodeMJpeg(buffer, result, FB_MPEG, 0);
      if(FB_MPEG != FB_8888){
        medianFilter(FB_MPEG, FB_8888, WW, HH);
        sceKernelDelayThread(3);
      }
      if(checkAmbientLight) {
        getMargRB(FB_8888, margRB);
      }
      YCbCr(FB_8888, B_WORK, SIZE_FBu8, margRB, threshNum);
      sceKernelDelayThread(3);
      if(gSize){
        i = 0;
        while(i<gSize){
          gridEval(B_WORK, grids[i].x, grids[i].y, grids[i].z);
          i++;
        }
      } sceKernelDelayThread(3);
      if(yMrg >= HH || xMrg >= WW) pOut =
      getBeInfo(B_WORK, xCont, yCont, WW-1, HH-1, xMin, yMin);
      else pOut = getFeInfo(B_WORK, xCont, yCont, WW-1, HH-1, xMin, yMin, xMrg, yMrg);
      sceKernelDelayThread(3);
      if(fb565) {
        convertBto565();
        sceKernelDelayThread(3);
      }
      run = true;
      scePowerTick(PSP_POWER_TICK_DISPLAY);
      if(wFSignalSema){
        semaState = true;
        sceKernelWaitSema(semaid , 1, 0);
        appSemaState = false;
        semaState = false;
      }
    }
    out_1:
      if(wFSignalSema) sceKernelSignalSema(semaid, 1);
      sceUsbCamStopVideo();
    out_2:
      run = false;
  }
  return 0;
}

void Uht::convertBto565(){
  u32 in;
  u8 r, v, b;
  
  u16 i = 0;
  while(i<SIZE_FBu8){
    in = FB_8888[i];
    r = ((u8)(in>>3)) & 0b11111;
    v = (u8)(in>>10);
    b = (u8)(in>>19);
    
    FB_565[i] = r|(v<<5)|(b<<11);
    i++;
  }
}

void Uht::LoadModules(){
  sceUtilityLoadUsbModule(PSP_USB_MODULE_ACC);
  sceUtilityLoadUsbModule(PSP_USB_MODULE_CAM);
  sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
}

void Uht::StartUsb(){
  sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBACC_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBCAM_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBCAMMIC_DRIVERNAME, 0, 0);
}

int Uht::InitJpegDecoder(){
  int result = sceJpegInitMJpeg();
  if (result < 0) return result;
  if(HH == 120) result = sceJpegCreateMJpeg(WW, 128);
  else result = sceJpegCreateMJpeg(WW, HH);
  return result;
}

void Uht::UnloadModules(){
  sceUtilityUnloadUsbModule(PSP_USB_MODULE_CAM);
  sceUtilityUnloadUsbModule(PSP_USB_MODULE_ACC);
  sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

void Uht::StopUsb(){
  sceUsbStop(PSP_USBCAMMIC_DRIVERNAME, 0, 0);
  sceUsbStop(PSP_USBCAM_DRIVERNAME, 0, 0);
  sceUsbStop(PSP_USBACC_DRIVERNAME, 0, 0);
  //sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
}

int Uht::FinishJpegDecoder(){
  int result = sceJpegDeleteMJpeg();
  if (result < 0) return result;

  result = sceJpegFinishMJpeg();
  if (result < 0) return result;
  
  return result;
}

void Uht::relativeCleaner(bool absClean){
  if(thid) sceKernelTerminateDeleteThread(thid);
  FinishJpegDecoder();
  sceUsbDeactivate(PSP_USBCAM_PID);
  StopUsb();
  if(absClean){
    UnloadModules();
  }
  if(wFSignalSema){
    sceKernelSignalSema(semaid, 1);
    sceKernelDeleteSema(semaid);
  }
}

Uht::~Uht(){
  relativeCleaner(!relativeClean);
}

