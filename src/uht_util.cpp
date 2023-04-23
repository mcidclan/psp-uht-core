// m-c/d
#include "uht_util.h"

#define W_ 160
#define H_ 120
static const u8 W2 = W_/2;
static const u8 H2 = H_/2;

void YCbCr(const u32* in, u8* out, const u16 size, const u8 margRB, const u8 tn){
  u16 i = 0;
  u8 Cb, Cr, Y;
  u8 r, v, b;
  u32 val;
  float f_1;
  bool evl = true;
  while(i<size) {
    val = in[i];
    r = (u8)val;
    v = (u8)(val>>8);
    b = (u8)(val>>16);
    if(margRB >= 32 && r > b) {
      f_1 = (256.0f-(float)(margRB))/48.0f;
      Cb = (u8)(((float)(r - b))/f_1);
      r-= Cb;
      b+= Cb;
    }
    if(tn == 3){
      Y = 0.299f*r+0.587f*v+0.114f*b - 128;
      Cr = r-Y;
      Cb = b-Y;
      evl = ((Cb>=77) && (Cb<=127) && (Cr>=133) && (Cr<=173));
    } else {
      Cb = -0.148f*r - 0.291f*v + 0.439f*b + 128.0f;
      Cr =  0.439f*r - 0.368f*v - 0.071f*b + 128.0f;
      if(tn == 0) evl = ((Cb>=85) && (Cb<=127) && (Cr>=135) && (Cr<=173)); else
      if(tn == 1) evl = ((Cb>=77) && (Cb<=135) && (Cr>=133) && (Cr<=180)); else
      if(tn == 2) evl = ((Cb>=69) && (Cb<=142) && (Cr>=131) && (Cr<=187));
    }
    if(evl) out[i] = 0; else out[i] = 255;
    i++;
  }
}

void getMargRB(const u32 *in, u32 &margRB){
  u32 pix;
  u8 px = 0;
  u16 py = 0;
  u16 pe = H_*W_; 
  u8 r, b;
  margRB = 0;
  while(py<pe){
    pix = in[px+py];
    r = ((u8)(pix));
    b = ((u8)(pix>>16));
    if (r > b) margRB += (r - b);
    px ++;
    if(px == W_){
      px = 0;
      py += W_;
    }
  }
  margRB = margRB/pe;
}

void medianFilter(const u32* in, u32* out, const u8 xend, const u8 yend){
  u8 x, y;
  u16 p, p0, p1, p2;
  u16 R, V, B;
  int Y;
  for(y = 0; y < yend; y++){
    for(x = 0; x < xend; x++){
      p = y*W_+x;
      if(y == 0 || y == H_-1 || x == 0 || x == W_-1){
        out[p] = in[p];
      } else {
        Y = -1;
        R = V = B = 0;
        while(Y<2){
          p1 = p+Y*W_;
          p0 = p1-1;
          p2 = p1+1;
          R += (u8)in[p0] + (u8)in[p1] + (u8)in[p2];
          V += (u8)(in[p0]>>8) + (u8)(in[p1]>>8) + (u8)(in[p2]>>8);
          B += (u8)(in[p0]>>16) + (u8)(in[p1]>>16) + (u8)(in[p2]>>16);
          Y+=1;
        }
        R /= 9;
        V /= 9;
        B /= 9;
        out[p] = R+(V<<8)+(B<<16);
      }
    }
  }
}

void gridEval(u8 *in, const u8 cXsize, const u8 cYsize, const u16 pThresh){
  u16 px, py, pe, acum;
  u8 x, y, s;
  for(x=0; x<W_; x+=cXsize){
    for(y=0; y<H_; y+=cYsize){
      px = x;
      py = y*W_;
      pe = py + cYsize*W_;
      acum = 0;
      while(py<pe){
        if(in[px+py]) acum += 1;
        px ++;
        if(px >= (x + cXsize)){
          px = x;
          py += W_;
        }
      }
      if(acum>pThresh) s = 0xFF; else s = 0x00;
      py = y*W_;      
      while(py<pe){
        in[px+py] = s;
        px ++;
        if(px >= (x + cXsize)){
          px = x;
          py += W_;
        }
      }
    }
  }
}

void getQtInfo(const u8* in, u8 *xcont, u8 *ycont, const u8 xend, const u8 yend){
  memset(xcont, 0x00, W_);
  memset(ycont, 0x00, H_);
  u16 p;
  u8 x, y;
  for(y = 1; y < yend; y++){
    for(x = 1; x < xend; x++){
      p = y*W_+x;
      if(!in[p]) {
        xcont[x]+=1;
        ycont[y]+=1;
      }
    }
  }
}

vec4i getBeInfo(const u8* in, u8 *xcont, u8 *ycont,
    const u8 xend, const u8 yend, const u8 xMin, const u8 yMin){
  getQtInfo(in, xcont, ycont, xend, yend);
  u8 Curr;
  u8 x, y, xE, xB, yE, yB;
  xE = xB = yE = yB = 0x0;
  x = 1;
  while(x<xend){
    Curr = xcont[x];
    if(xB && Curr>=yMin) xE = x;
    if(!xE && Curr>=yMin) xB = x;
    x++;
  }
  y = 1;
  while(y<yend){
    Curr = ycont[y];
    if(yB && Curr>=xMin) yE = y;
    if(!yE && Curr>=xMin) yB = y;
    y++;
  }
  vec4i out = {xB, xE ,yB, yE};
  return out;
}

vec4i getFeInfo(const u8* in, u8 *xcont, u8 *ycont,
    const u8 xend, const u8 yend,
    const u8 xMin, const u8 yMin,
    const u8 xMrg, const u8 yMrg){
  getQtInfo(in, xcont, ycont, xend, yend);
  u8 x = 1, y = 1, xE = 0, xB = 0, yE = 0, yB = 0;
  u8 acm_0 = 0;
  u8 acm_1 = 0;
  while(x < xend){
    if(xcont[x] >= yMin){
      if(acm_0 >= xMin) xE = x;
      if((acm_1 > xMrg) || (acm_1 == (x - 1))) {
        acm_0 = 0;
        xB = x;
      } acm_0 ++;
      acm_1 = 0;
    } else acm_1 ++;
    x++;
  }
  if(!xE) xB = 0;
  acm_0 = 0;
  acm_1 = 0;
  while(y < yend){
    if(ycont[y] >= xMin){
      if(acm_0 >= yMin) yE = y;
      if((acm_1 > yMrg) || (acm_1 == (y - 1))) {
        acm_0 = 0;
        yB = y;
      } acm_0 ++;
      acm_1 = 0;
    } else acm_1 ++;
    y++;
  }
  if(!yE) yB = 0;
  vec4i out = {xB, xE ,yB, yE};
  return out;
}

void getAbsPosition(const vec4i in, vec2i *out){
  out->x = in.x+(in.y-in.x)/2;
  out->y = in.z+(in.w-in.z)/2;
};

void getOrtPosition(const vec4i in, vec2i *out){
  getAbsPosition(in, out);
  out->x -= W2;
  out->y -= H2;
};

void getSqrDimension(const vec4i sqr, vec2i *out){
  out->x = sqr.y-sqr.x;
  out->y = sqr.w-sqr.z;
}

float evalStdHand(const vec4i sqr){
  vec2i dim;
  getSqrDimension(sqr, &dim);
  if(!dim.x) return 0.0f;
  return (((float)dim.y)/((float)dim.x))*100.0f;
}

