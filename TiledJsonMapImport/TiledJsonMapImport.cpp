/******************************************************************************
* Load Tiled JSON maps demo project
* GameMap.* uses library json11 ( MIT license) to load json files
* map data from json file is saved into a GameMap_t struct
* (json11 : github.com/dropbox/json11)
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "DxLib.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include "GameMap.h"
/******************************************************************************/
#define MSPF (1000.0/60.0)
void static computeFrameRate(int now);
float getRuntimeFPS();

/******************************************************************************/
/******************************************************************************/
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR args, int) {

#ifdef _DEBUG
  //コンソールを開く
  FILE* fp;
  AllocConsole();
  freopen_s(&fp, "CONOUT$", "w", stdout);
#endif

  /*Read filename argument*/
  char mapPath[1024] = "map.json"; //default
  if (strlen(args) > 0) strncpy_s(mapPath, args, sizeof(mapPath));

  /*DxLib*/
  int windowMode = TRUE;
  ChangeWindowMode(windowMode); // ウィンドウモードに設定
  DxLib_Init();   // DXライブラリ初期化処理

  /*load fonts*/
  int uiFontHandle = CreateFontToHandle(NULL, 16, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
  int errFontHandle = CreateFontToHandle(NULL, 12, -1, DX_FONTTYPE_ANTIALIASING_EDGE);

  /*load json*/
  GameMap_t* map = GameMap_loadFromTiledJSON(mapPath);
  GameMap_print(map);

  double mx = 0, my = 0; //Position on map (mx,my)

  /***********Main Loop****************/
  while (1) {

    int startTime = GetNowCount();
    computeFrameRate(startTime);

    /** Input **/

    /*Exit*/
    if (CheckHitKey(KEY_INPUT_ESCAPE) == 1)
    { break;  }

    /*Fullscreen*/
    if (CheckHitKey(KEY_INPUT_RETURN) == 1) {
      windowMode = windowMode == TRUE ? FALSE : TRUE;
      ChangeWindowMode(windowMode);
      uiFontHandle = CreateFontToHandle(NULL, 16, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
      errFontHandle = CreateFontToHandle(NULL, 12, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
      GameMap_free(map);
      map = GameMap_loadFromTiledJSON(mapPath);
      GameMap_print(map);
    }

    /*Reload map*/
    if (CheckHitKey(KEY_INPUT_R) == 1){
      GameMap_free(map);
      map = GameMap_loadFromTiledJSON(mapPath);
      GameMap_print(map);
    }
    /*Movement*/
    if (CheckHitKey(KEY_INPUT_DOWN) == 1) my += 0.2;
    if (CheckHitKey(KEY_INPUT_UP) == 1) my -= 0.2;
    if (CheckHitKey(KEY_INPUT_LEFT) == 1) mx -= 0.2;
    if (CheckHitKey(KEY_INPUT_RIGHT) == 1) mx += 0.2;

    /*Draw*/
    ClearDrawScreen();
    if(map != nullptr) DrawGameMap(map, mx, my);

    DrawStringToHandle(0, 400, "* Key [R]: Reload map\n* Use Arrow keys to move", 0xFFFFFF, uiFontHandle, 0x00);
    DrawFormatStringToHandle(440, 10, 0xFFFFFF, uiFontHandle, "%.2f FPS\nPos(x %.1f y %.1f)",getRuntimeFPS(), mx, my);
    if (map == nullptr) DrawFormatStringToHandle(32, 64, 0xFFFFFF, errFontHandle, "%s", GameMap_getErrStr());

    /*Timming and sync*/
    int elapsedTime = GetNowCount() - startTime;
    if (elapsedTime < MSPF)
      WaitTimer(MSPF - elapsedTime);

  }
  /************************************/

  DxLib_End();   // DXライブラリ終了処理
  return 0;
}

/******************************************************************************/
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
