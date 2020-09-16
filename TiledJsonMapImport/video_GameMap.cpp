/*******************************************************************************
 * GameMap Implementation that DEPENDS on DxLib (or other graphic lib)
*******************************************************************************/

#define VIDEO_GAME_MAP_C

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include "DxLib.h"
#include "GameMap.h"
#include "_GameMap.h"


/*******************************************************************************
*******************************************************************************/
/**
 * Load maps graphic data
 * Depends on a graphic library (DxLib)
 */
int ReloadGameMapGraphs(GameMap_t* map)
{
  int rc = 0; //return code
  map->tileHandlesNum = 0;

  //Compute number of tile handles required
  int maxgid = -1;
  int maxgidTileset = -1;
  for (int i = 0; i < map->tilesetsNum; i++){
    if (map->tilesets[i].firstgid > maxgid) {
      maxgid = map->tilesets[i].firstgid;
      maxgidTileset = i;
    }
  }
  map->tileHandlesNum =   map->tilesets[maxgidTileset].firstgid
                        + map->tilesets[maxgidTileset].tilecount;

  //Allocate and load all graphs
  map->tileHandles = (int *)calloc(map->tileHandlesNum, sizeof(int));
  for (int i = 0; i < map->tilesetsNum; i++) {
    GMapTileset_t *tileset = &map->tilesets[i];
    int res = 0;
    int Ynum = tileset->imageheight / tileset->tileheight;
    int Xnum = tileset->imagewidth / tileset->tilewidth;
    //
    int* handles = map->tileHandles + tileset->firstgid;
    res = LoadDivGraph(tileset->imgPath, tileset->tilecount, Xnum, Ynum,  
                      tileset->tilewidth, tileset->tileheight, handles);
    if (res == -1){
      printf("ERROR %s::line %d: can not load \"%s\"\n",
              __func__, __LINE__, tileset->imgPath);
      rc = 1;
    }
  }
  return rc;
}
/*******************************************************************************
*******************************************************************************/
void DeleteGameMapGraphs(GameMap_t* map)
{
  if (map->tileHandlesNum <= 0 || map->tileHandles == nullptr)
    return;

  for (int i = 0; i < map->tileHandlesNum; i++){
    if(map->tileHandles[i] > 0)
      DeleteGraph(map->tileHandles[i]);
  }
  free(map->tileHandles);
  map->tileHandles = nullptr;
}
/*******************************************************************************
*******************************************************************************/
/**
* Draw GameMap with camera position at pixel (x,y)
*/

/*
  TODO: REWRITE AND OPTIMIZE
*/
static const int _MAP_SCREEN_SIZE_W = 640;
static const int _MAP_SCREEN_SIZE_H = 480;


void DrawGameMap(const GameMap_t* map, double x, double y)
{
  int tw = map->tilewidth, th = map->tileheight;
  int tw2 = tw / 2, th2 = th / 2;
  int px = -x * map->tilewidth;
  int py = -y * map->tileheight;
  for (int i = 0; i != map->height; i++ , py += th)
  {
    if (py < -th || py > _MAP_SCREEN_SIZE_H) continue;
    px = -x * map->tilewidth;
    for (int j = 0; j != map->width; j++, px += tw)
    {
      if (px < -tw || px > _MAP_SCREEN_SIZE_W) continue;
      for (int l = 0; l != map->layersNum; l++)
      {
        if (map->layers[l].visible == 0) continue;
        if (i >= map->layers[l].height) continue;
        if (j >= map->layers[l].width) continue;

        unsigned int ti = map->layers[l].data[i * map->width + j];
        int _handle = map->tileHandles[ti & _GID_MASK];
        if (_handle == 0) continue;

        double flipY = 1;
        double flipX = 1;
        double rot = 0;

        unsigned tflags = ti & _FLAGS_MASK;
        switch (tflags)
        {
        case 0:
          /*fast common case*/
          DrawGraph(px, py, _handle, TRUE);
          break;

        case _FLAGS_MASK:
          flipX = -1;
        case _RIGHT_ROTA_FLAG: //right rotation
          rot = M_PI / 2.0;
          DrawRotaGraph3(px + tw2, py + th2, tw2, th2, flipX, flipY, rot, _handle, TRUE, FALSE);
          break;

        case _LEFT_ROTA_FLAG: //left rotation
          rot = -M_PI / 2.0;
          DrawRotaGraph3(px + tw2, py + th2, tw2, th2, flipX, 1, rot, _handle, TRUE, FALSE);
          break;

        case _FLIPPED_D_FLAG: //anti-diagonal flip
          rot = M_PI / 2.0;
          flipY = -1;
          DrawRotaGraph3(px + tw2, py + th2, tw2, th2, 1, flipY, rot, _handle, TRUE, FALSE);
          break;

        default: //X and Y Flips
          flipY *= (ti & _FLIPPED_V_FLAG) == 0 ? 1 : -1;
          flipX *= (ti & _FLIPPED_H_FLAG) == 0 ? 1 : -1;
          DrawRotaGraph3(px + tw2, py + th2, tw2, th2, flipX, flipY, 0, _handle, TRUE, FALSE);
          break;
        }
      }/*for l*/
    }/*for j*/
  }/*for i*/
  return;
}
