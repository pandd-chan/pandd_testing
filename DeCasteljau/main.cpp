/******************************************************************************
  Draw and design curves in an easy way with "De Casteljau" algorithm: 
  https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm

  Input: 
    plist: list of points plist
    t :    curve position in range [0,1], t == 0 -> curve start point, t == 1 -> curve end point
  WHILE more than one point in "plist"
    DO:
      plist[i] = plist[i]*(1-t) + t * plist[i+1];
      remove last point from plist;

  Note: The resulting curve is the same as the Bézier curve for the given points.
  Note: time complexity O(N^2), but really easy to implement.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "DxLib.h"
#define _USE_MATH_DEFINES
#include <math.h>

#define MSPF (1000.0/60.0)
#define SCREEN_W 640
#define SCREEN_H 480

/******************************************************************************
  Points
******************************************************************************/
typedef struct { double x; double y; } Point;
const int MAX_POINTS_NUM = 256;
int pointsNum = 0;
Point pointsList[MAX_POINTS_NUM];

/*****************************************************************************/
/**
  @param points : curve control points
  @param pointsNum: number of points
  @param t : position at curve, inside range [0,1]
  @return position at curve(t)
*/
Point deCasteljau(const Point* points, int pointsNum, double t)
{
  int n = pointsNum;
  static Point _auxList[MAX_POINTS_NUM];
  const Point* pList = points;
  while (n > 1){
    n--;
    for (int i = 0; i < n; i++) {
      _auxList[i].x = pList[i].x * (1 - t) + t * pList[i + 1].x;
      _auxList[i].y = pList[i].y * (1 - t) + t * pList[i + 1].y;
    }
    pList = _auxList; //in the first iteration we use pointsNum as input, then use _auxList
  }
  return _auxList[0];
}

/*****************************************************************************/

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  const int NEKO_FRAMES_NUM = 2;
  int nekoGraph[NEKO_FRAMES_NUM];
  int fontHandle;

#ifdef _DEBUG
  //コンソールを開く
  FILE* fp;
  AllocConsole();
  freopen_s(&fp, "CONOUT$", "w", stdout);
#endif

  /*DxLib*/
  int windowMode = TRUE;
  SetMouseDispFlag(TRUE);
  ChangeWindowMode(windowMode); // ウィンドウモードに設定
  DxLib_Init();   // DXライブラリ初期化処理

  fontHandle = CreateFontToHandle(NULL, 12, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
  nekoGraph[0] = LoadGraph("oneko_0.png");
  nekoGraph[1] = LoadGraph("oneko_1.png");

  SetBackgroundColor(255, 255, 255);
  while (1) {

    int startTime = GetNowCount();

    /*  Input  ***********************/

    //Quit
    if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) 
    { break; }

    //Fullscreen
    if (CheckHitKey(KEY_INPUT_RETURN) == 1) {
      windowMode = windowMode == TRUE ? FALSE : TRUE;
      ChangeWindowMode(windowMode);
      SetMouseDispFlag(TRUE);
      fontHandle = CreateFontToHandle(NULL, 12, -1, DX_FONTTYPE_ANTIALIASING_EDGE);
      nekoGraph[0] = LoadGraph("oneko_0.png");
      nekoGraph[1] = LoadGraph("oneko_1.png");
    }

    //Mouse
    static int mouseLeftPressed = 0;
    if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) mouseLeftPressed++;
    else mouseLeftPressed = 0;

    if (mouseLeftPressed == 1){
      int mx, my;
      GetMousePoint(&mx, &my);
      Point p = { mx, my };
      pointsList[pointsNum++] = p;
    }

    /*  Draw  ************************/

    //Draw grid
    ClearDrawScreen();
    unsigned int gridColor = GetColor(200, 200, 200);
    for (int i = 0; i < SCREEN_W; i += 40) DrawLine( i ,0 ,i , SCREEN_H , gridColor) ;
    for (int i = 0; i < SCREEN_H; i += 40) DrawLine(0, i, SCREEN_W, i, gridColor);

    //Draw points
    unsigned int circleColor = GetColor(255, 0, 0);
    unsigned int textColor = GetColor(255, 255, 255);
    for (int textY = 20 , i = 0; i < pointsNum; i++, textY += 20) {
      DrawCircle(pointsList[i].x, pointsList[i].y, 10, circleColor, 0, 1);
      DrawFormatStringToHandle(500, textY, textColor, fontHandle, "(%02d,%02d)", (int)pointsList[i].x, (int)pointsList[i].y);
    }

    //Draw curve
    Point p1, p2, offset = { 0,0 };
    p1 = deCasteljau(pointsList, pointsNum, 0);
    for (double t = 0; t < 1; t += 0.05/(1+pointsNum))
    {
      p2 = deCasteljau(pointsList, pointsNum, t);
      DrawLine(p1.x, p1.y, p2.x, p2.y, 0x00);
      offset.x += fabs(p2.x - p1.x);
      offset.y += fabs(p2.y - p1.y);
      p1 = p2;
    }
    //Total length of the curve (estimation)
    double pathLength = sqrt(offset.x * offset.x + offset.y * offset.y);

    //draw cat
    double nekoSpeed = 2.0 / (1+pathLength);
    static double nekoT = 0.0;
    nekoT = fmod(nekoT + nekoSpeed, 1.0);
    static double nekoFrame;
    nekoFrame = fmod(nekoFrame + 0.1, NEKO_FRAMES_NUM);
    //
    Point nekoPos;
    nekoPos = deCasteljau(pointsList, pointsNum, nekoT);
    DrawRotaGraph(nekoPos.x, nekoPos.y, 1, 0, nekoGraph[(int)floor(nekoFrame)], 1, 0, 0);

    /*Timming and sync ***************/

    int elapsedTime = GetNowCount() - startTime;
    if (elapsedTime < MSPF)
      WaitTimer(MSPF - elapsedTime);
  }

  DxLib_End();   // DXライブラリ終了処理
  return 0;
}