
/*
  DXLIBRARY "per pixel" image effects rendering,
  using TGA images as memory buffers and CreateGraphFromMem() to render effects
  */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "DxLib.h"
#define _USE_MATH_DEFINES
#include <math.h>

//disable warning
//warning C4244: conversion from 'double' to 'int', possible loss of data
#pragma warning( disable : 4244 ) 

/******************************************************************************/
/******************************************************************************/

struct t_TGAGraph {
  uint8_t* data;
  uint8_t* imgdata;
  int valid; //0 = invalid, != 0 valid img
  int size;
  int w;
  int h;
  int pixelDepth;
};
typedef struct t_TGAGraph t_TGAGraph;

/*************************************/
/**
   Read TGA file into memory
   NOTE: TGA file has to be uncompressed and use 32bits color depth( RGBA )
   @return -1 if can't load file
   @return 0  if TGA loaded
*/

//format info: https://www.fileformat.info/format/tga/egff.htm
#define TGA_IMGDATA_OFFSET 0x12
#define TGA_IMG_WIDTH_OFFSET 0x0C //WORD
#define TGA_IMG_HEIGHT_OFFSET 0x0E //WORD
#define TGA_IMG_PXDEPTH_OFFSET 0x0F //BYTE

int loadTGAfile(const char* filename, t_TGAGraph * img)
{
  FILE* fp;
  if (fopen_s(&fp, filename, "rb")) {
    img->valid = 0;
    return -1;
  }
  //
  fseek(fp, 0, SEEK_END);
  size_t fileSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  uint8_t* filedata = (uint8_t*)malloc(fileSize);
  //
  size_t readCount = fread(filedata, 1, fileSize, fp);
  img->size = readCount;
  img->w = *(uint16_t*)(filedata + TGA_IMG_WIDTH_OFFSET);
  img->h = *(uint16_t*)(filedata + TGA_IMG_HEIGHT_OFFSET);
  img->pixelDepth = 4;
  img->data = filedata;
  img->imgdata = img->data + TGA_IMGDATA_OFFSET;
  printf("%s : %dx%d %d bits per pixel, size %d\n", filename,
    img->w, img->h, img->pixelDepth * 8, img->size);
  img->valid = 1;
  return 0;
}
/**/
void freeTGAfile(t_TGAGraph* img)
{
  free(img->data);
  img->data = img->imgdata = NULL;
  img->valid = 0;
}

/******************************************************************************/
/******************************************************************************/

/*
   MAIN IDEA:
    SRC IMAGE -> || TRANSFORMATION || -> OUTPUT IMAGE
    pixel-level access makes complex effects possible(  but this is CPU expensive $$$)
*/

void flagEffect(const t_TGAGraph* src, t_TGAGraph* dest);
void waterDropEffect(const t_TGAGraph* src, t_TGAGraph* dest, double centerX, double centerY);
void tunnelEffect(const t_TGAGraph* src, t_TGAGraph* dest);

#define MSPF (1000.0/60.0)
void static computeFrameRate(int now); /* compute framerate*/
float getRuntimeFPS();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#ifdef _DEBUG
  //コンソールを開く
  FILE* fp;
  AllocConsole();
  freopen_s(&fp, "CONOUT$", "w", stdout);
#endif

  /*DxLib*/
  int windowMode = TRUE;
  ChangeWindowMode(windowMode); // ウィンドウモードに設定
  DxLib_Init();   // DXライブラリ初期化処理
  int fontHandle;
  fontHandle = CreateFontToHandle(NULL, 32, -1, DX_FONTTYPE_ANTIALIASING_EDGE);

  /*Load images*/
  t_TGAGraph canvas;
  t_TGAGraph tile;
  loadTGAfile("canvas.tga", &canvas);
  loadTGAfile("tile.tga", &tile);
  if (!canvas.valid) { printf("Can't load %s\n", "canvas.tga"); return 1; };
  if (!canvas.valid) { printf("Can't load %s\n", "tile.tga"); return 1; };
  int CanvasHandle = CreateGraphFromMem(canvas.data, canvas.size);

  /*demo control vars*/
  int timer = 0, effect = 0;
  const int TIMER_DELAY = 100;
  const int EFFECTS_NUM = 3;
  const char* effectName[EFFECTS_NUM] = {
    "Flag Effect", "Tunnel" , "Water drop",
  };
  double dropX = 0.5, dropY = 0.5; //water drop position
  dropY = dropX = 0.1 + 0.6 * ((double)rand() / RAND_MAX); //from [0.2 to 0.6]

  while (1) {
    int startTime = GetNowCount();
    computeFrameRate(startTime);
    /*Input*/
    if (CheckHitKey(KEY_INPUT_ESCAPE) == 1)
    {
      break;
    }
    if (CheckHitKey(KEY_INPUT_RETURN) == 1) {
      windowMode = windowMode == TRUE ? FALSE : TRUE;
      ChangeWindowMode(windowMode);
      fontHandle = CreateFontToHandle(NULL, 32, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
    }

    /*Render effect to canvas*/

    switch (effect) {
    case 0: flagEffect(&tile, &canvas); break;
    case 1: tunnelEffect(&tile, &canvas); break;
    case 2: waterDropEffect(&tile, &canvas, dropX, dropY); break;
    default: flagEffect(&tile, &canvas);
    }

    /*Make new graph from canvas*/
    DeleteGraph(CanvasHandle);
    CanvasHandle = CreateGraphFromMem(canvas.data, canvas.size);

    /*Update screen*/
    DrawGraph(0, 0, CanvasHandle, FALSE);
    DrawFormatStringToHandle(440, 10, 0xFFFFFF, fontHandle, "%.2f FPS", getRuntimeFPS());
    DrawStringToHandle(10, 10, *(effectName + effect), 0xFFFFFF, fontHandle);
    //ScreenFlip();

    /*Timming and sync*/
    int elapsedTime = GetNowCount() - startTime;
    if (elapsedTime < MSPF)
      WaitTimer(MSPF - elapsedTime);

    /*Update Effects and logic timers*/
    timer = (timer + 1) % (TIMER_DELAY);
    if (timer == 0) {
      effect = (effect + 1) % EFFECTS_NUM;
      dropY = dropX = 0.1 + 0.6 * ((double)rand() / RAND_MAX); //from [0.2 to 0.6]
    }
  }
  DxLib_End();   // DXライブラリ終了処理
  return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/* compute framerate*/
static float _runtimeFPS = 0.0f;
void static computeFrameRate(int now)
{
  static int frameCount = 0;
  static int FRAME_RESOLUTION = 10; /*fps value update*/
  frameCount++;
  if (frameCount < FRAME_RESOLUTION)
    return;
  frameCount = 0;
  static int before = 0;
  int elapsed = (now - before);
  if (elapsed == 0) _runtimeFPS = -1;
  else _runtimeFPS = (1000.0 * FRAME_RESOLUTION) / (float)elapsed;
  before = now;
}
float getRuntimeFPS()
{
  return _runtimeFPS;
}


/*****************************************************************************/
/* 
  Transformation/Effects functions 
  TODO:
    *Optimize
    *play arround with more effects
*/
/*****************************************************************************/

void flagEffect(const t_TGAGraph* src, t_TGAGraph* dest)
{
  const uint8_t* srcData = src->imgdata;
  uint8_t* destData = dest->imgdata;
  int sw = src->w, sh = src->h;
  int dw = dest->w, dh = dest->h;
  static double fi = 0.0;
  fi = fmod(fi + 0.04, 2 * M_PI);
  double alpha = fi;
  double alphaSin, alphaCos;
  int dx, dy, sx, sy;
  for (dy = 0; dy < dh; dy++) {
    alphaSin = sin(alpha);
    alpha += 0.012;
    for (dx = 0; dx < dw; dx++) {
      alphaCos = cos(alpha);
      sx = (sw + (int)(dx + 40 * alphaSin)) % sw;
      sy = (sh + (int)(dy + 20 * alphaCos)) % sh;
      double colorLevel = alphaSin > 0 ? 1.0 : 1.0 + 0.4 * alphaSin;
      //Copy color data
      long dpix = (dy * dw + dx) * dest->pixelDepth;
      long spix = (sy * sw + sx) * src->pixelDepth;
      destData[dpix + 0] = srcData[spix + 0] * colorLevel;
      destData[dpix + 1] = srcData[spix + 1] * colorLevel;
      destData[dpix + 2] = srcData[spix + 2] * colorLevel;
      destData[dpix + 3] = srcData[spix + 3] * colorLevel;
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
void waterDropEffect(const t_TGAGraph* src, t_TGAGraph* dest, double centerX, double centerY)
{
  const uint8_t* srcData = src->imgdata;
  uint8_t* destData = dest->imgdata;
  int sw = src->w, sh = src->h;
  int dw = dest->w, dh = dest->h;
  static double alpha = 0.1;
  alpha = fmod(alpha + 0.05, 2 * M_PI);
  double radX, radY;
  float ddw = dw, ddh = dh;
  int dx, dy, sx, sy;
  for (dy = 0; dy < dh; dy++) {
    double radY = fabs(dy / ddh - centerY);
    for (dx = 0; dx < dw; dx++) {
      double radX = fabs(dx / ddw - centerX);
      double radNorm = sqrt(radX * radX + radY * radY);
      double radUx = radX / radNorm;
      double radUy = radY / radNorm;
      double waterLevel = 20 * sin(2 * (2 * M_PI * radNorm) + alpha) + 1e-10;
      //printf("%fl\n", waterLevel);
      sx = dx + (1.0 / waterLevel) * radUx + (1 - radX) * waterLevel * radUx;
      sy = dy + (1.0 / waterLevel) * radUy + (1 - radY) * waterLevel * radUy;
      //sy = dy;
      sx %= sw;
      sy %= sh;
      //Copy color data
      long spix = (sy * sw + sx) * src->pixelDepth;
      dx--;
      for (int r = 0; r != 3 && dx < dw; r++) {
        dx++;
        long dpix = (dy * dw + dx) * dest->pixelDepth;
        destData[dpix + 0] = srcData[spix + 0];
        destData[dpix + 1] = srcData[spix + 1];
        destData[dpix + 2] = srcData[spix + 2];
        destData[dpix + 3] = srcData[spix + 3];
      }
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
//https://lodev.org/cgtutor/tunnel.html
void tunnelEffect(const t_TGAGraph* src, t_TGAGraph* dest)
{
  const uint8_t* srcData = src->imgdata;
  uint8_t* destData = dest->imgdata;
  int sw = src->w, sh = src->h;
  int dw = dest->w, dh = dest->h;
  //center coordinates (normalized)
  double torgX = 0.5;
  double torgY = 0.5;
  double ddw = dw;
  double ddh = dh;
  const double DEPTH = 0.2;
  static double scroll = 0.0;
  scroll = fmod(scroll + 0.01, 1.0);
  int dx, dy, sx, sy; // pixel coordinates
  double ndx, ndy, nsx, nsy; //normalized [0,1]
  for (dy = 0; dy < dh; dy++) {
    ndy = dy / ddh - torgY;
    double sqdy = ndy * ndy;
    for (dx = 0; dx < dw; dx++) {
      //
      ndx = dx / ddw - torgX;
      double dist = sqrt(ndx * ndx + sqdy);
      //compute X
      nsx = atan2(ndy, ndx) / (2 * M_PI);
      sx = (nsx + torgX) * dw;
      //sx = dx;
      sx %= sw;
      //compute Y
      nsy = DEPTH * 1.0 / dist;
      sy = (nsy + torgY + scroll) * dh;
      //sy = dy;
      sy %= sh;
      //Copy color data
      long spix = (sy * sw + sx) * src->pixelDepth;
      dx--;
      for (int r = 0; r != 3 && dx < dw; r++) {
        dx++;
        long dpix = (dy * dw + dx) * dest->pixelDepth;
        destData[dpix + 0] = srcData[spix + 0] * dist;
        destData[dpix + 1] = srcData[spix + 1] * dist;
        destData[dpix + 2] = srcData[spix + 2] * dist;
        destData[dpix + 3] = srcData[spix + 3] * dist;
      }
    }
  }
}
