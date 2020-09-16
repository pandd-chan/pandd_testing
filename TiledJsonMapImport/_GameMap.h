/*******************************************************************************
 * GameMap
 * Private header
*******************************************************************************/

#ifndef __GAME_MAP_H
#define __GAME_MAP_H

#include <string>

/*******************************************************************************/
//Bit masks for tile values
static const unsigned _FLIPPED_H_FLAG = 0x80000000;
static const unsigned _FLIPPED_V_FLAG = 0x40000000;
static const unsigned _FLIPPED_D_FLAG = 0x20000000;
static const unsigned _LEFT_ROTA_FLAG = _FLIPPED_D_FLAG | _FLIPPED_V_FLAG;
static const unsigned _RIGHT_ROTA_FLAG = _FLIPPED_D_FLAG | _FLIPPED_H_FLAG;
static const unsigned _FLAGS_MASK = _FLIPPED_H_FLAG | _FLIPPED_V_FLAG | _FLIPPED_D_FLAG;
static const unsigned _GID_MASK = ~_FLAGS_MASK;

/*******************************************************************************/
typedef struct GMapTileset_t GMapTileset_t;
typedef struct GMapTilelayer_t GMapTilelayer_t;

/**
 Load map graphics data
 NOTE: Call when need to reload graphics, for example when DXlib changes video mode
*/
int ReloadGameMapGraphs(GameMap_t* map);
/*
  Delete map graphs
*/
void DeleteGameMapGraphs(GameMap_t* map);

void _GameMap_appendToErrStr(const char* str);
void _GameMap_appendToErrStr(std::string str);
void _GameMap_clearErrStr();

struct GameMap_t{

  size_t byteSize;        //Total bytes size

  int width;              //Number of Map columns
  int height;             //Number of Map rows

  int tilewidth;          //Map grid width
  int tileheight;         //Map grid height

  int layersNum;        //Total Number of layers
  int tilesetsNum;      //Total Number of tilesets
  int objectsNum;       //Total Number of objects

  GMapTileset_t   *tilesets;      // Array of Tilesets
  GMapTilelayer_t *layers;        // Array of Layers
                                  // *Stored in render order,
                                  // bottom: layers[0] ----> top: layers[layersNum]

  //GMapProperties_t *properties;

  //Depends on DxLib
  int* tileHandles; //DXlib Graph Handles
  int tileHandlesNum;
};


/*******************************************************************************
*******************************************************************************/

#define _LAYER_NAME_MAXLEN 128
struct GMapTilelayer_t{

  int id;                              //unique across all layers
  char name[_LAYER_NAME_MAXLEN];       //Name assigned to this layer
  size_t nameHash;
  unsigned int *data;
  int width;
  int height;

  int offsetx; //Horizontal layer offset in pixels 
  int offsety; //Vertical layer offset in pixels
  double opacity; //Value between 0 and 1
  unsigned int visible; //FALSE == 0, TRUE != 0

  //uint32 tintcolor
  //uint32 transparentcolor

  //GMapProperties_t *properties;

};

/*******************************************************************************
*******************************************************************************/

#define _TILESET_NAME_MAXLEN 128
#define _TILESET_FILEPATH_LEN 256
struct GMapTileset_t{
  /**/
  char name[_TILESET_FILEPATH_LEN];
  char imgPath[_TILESET_FILEPATH_LEN];  //Image file path

  int firstgid;

  int imageheight;
  int imagewidth;

  int tilewidth;
  int tileheight;
  int tilecount;
};


#endif
