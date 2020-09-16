/*******************************************************************************
 * GameMap
*******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <iostream>
#include <vector>
//
#include "GameMap.h"
#include "_GameMap.h"
#include "json11.hpp"

using namespace std;

static std::string _GameMap_errStr;

/*
Private functions
*/
static std::string _getDir(const char* path); //Extract file dir from path
static std::vector<json11::Json> _getLayers(const json11::Json* map);
static int _loadMapLayers(const json11::Json* jsonMap, GameMap_t* map);
static void _freeMapLayers(GameMap_t *map);
static int _loadMapTilesets(const json11::Json* jsonMap, GameMap_t* map , const char* baseDir);
static void _freeMapTilesets(GameMap_t* map);
static void _printTilemapData(const unsigned int* data, int w, int h);
static int _findLayer(const GameMap_t* map, const char* layerName); //return -1 if layer not found

/*******************************************************************************/
/**
 * Load game map from Tiled JSON file
 * 
 * @return != 0 : error loading file
 * @return 0 : file load succesfully
 */
GameMap_t *GameMap_loadFromTiledJSON(const char *path)
{
  _GameMap_clearErrStr();
  GameMap_t* map;
  if(path == nullptr){
    return nullptr;
  }

  //copy json file contents to std::string
  std::ifstream fin(path);
  std::stringstream ss;
  ss << fin.rdbuf();
  fin.close();
  if (ss.str() == "") {
    _GameMap_appendToErrStr("Can not open file: \"");
    _GameMap_appendToErrStr(path);
    _GameMap_appendToErrStr("\"\n");
    return nullptr;
  }

  //parse json string
  std::string errmsg;
  json11::Json jsonMap;
  jsonMap = json11::Json::parse(  ss.str() , errmsg );

  //check parsing errors
  if( errmsg.size() != 0){
    _GameMap_appendToErrStr(path + (std::string)"\nJSON Parse Error:" + errmsg + "\n" );
    return nullptr;
  }

  //allocate map
  map = (GameMap_t*)calloc(1, sizeof(GameMap_t));
  if (map == nullptr || path == nullptr) {
    return nullptr;
  }
  map->byteSize += sizeof(GameMap_t);

  //fill struct with jsonMap data
  map->width = jsonMap["width"].int_value();
  map->height = jsonMap["height"].int_value();
  map->tileheight = jsonMap["tileheight"].int_value();
  map->tilewidth = jsonMap["tilewidth"].int_value();

  std::string mapDir = _getDir(path);
  int rc = 0; //return code
  if (rc == 0) rc = _loadMapLayers(&jsonMap, map);
  if (rc == 0) rc = _loadMapTilesets(&jsonMap, map , mapDir.c_str());
  if (rc == 0) rc = ReloadGameMapGraphs(map);

  if (rc != 0) {
    GameMap_free(map);
    return nullptr;
  }

  return map;
}

/*******************************************************************************/
void GameMap_free(GameMap_t *map)
{
  if (map == nullptr)
    return;
  DeleteGameMapGraphs(map);
  _freeMapLayers(map);
  _freeMapTilesets(map);
  free(map);
  return;
}

/*******************************************************************************/
unsigned int GameMap_getTileId(const GameMap_t* map, const char* layerName, int tx, int ty)
{
  int l =  _findLayer(map, layerName);
  return GameMap_getTileId(map, l, tx, ty);
}

unsigned int GameMap_getTileId(const GameMap_t* map, int layerId, int tx, int ty)
{
  int l = layerId;
  if (l < 0 || l >= map->layersNum) return 0;

  //Check for tx,ty out of map
  if (tx < 0 || ty < 0 || tx >= map->layers[l].width || ty >= map->layers[l].height)
    return 0;
  //Return tile GID
  return _GID_MASK & map->layers[l].data[ty * map->layers[l].width + tx];
}

/*******************************************************************************/
//@return -1 if layer not found
static int _findLayer(const GameMap_t* map, const char* layerName)
{
  std::string _nameString = layerName;
  hash<std::string> _hasher;
  size_t _nameHash = _hasher(_nameString);

  //Search layer
  int l = 0;
  for (l = 0; l < map->layersNum; l++)
    if (_nameHash = map->layers[l].nameHash) break;
  if (l == map->layersNum) l = -1; //Layer not found
  return l;
}


/*******************************************************************************/
/**
*
*/
static std::string _getDir(const char* path)
{
  std::string dir = path;
  std::size_t last = dir.find_last_of("\\/");
  if (last == string::npos){
    dir = "";
    return dir;
  }
  return dir.substr(0, last)+"/";
}

/*******************************************************************************/
/**
 * _getLayers():
 *  Get a list (layerList) with pointers to all layers
 * NOTE:
 * - Layers in JSON can be stored inside another layers (layer groups)
 * - Layers are stored in a tree structure
 */
#define _LAYER_TREE_MAX_DEPTH 10
static void _getLayers(const json11::Json * layer,
  std::vector<json11::Json > * layerList, int depth = 0)
{
  /*Recursion base case:*/
  /*layer is NOT a group, ADD layer pointer to list and RETURN*/
  if (true == (*layer)["layers"].is_null()
    && "group" != (*layer)["type"].string_value()) {
    layerList->push_back(*layer);
    return;
  }

  /*Limit recursion depth to 10 levels*/
  if (depth >= _LAYER_TREE_MAX_DEPTH)
    return;

  /*Recursion General case:*/
  /*layer is a group, go down a level*/
  json11::Json::array _layersArray;
  _layersArray = (*layer)["layers"].array_items();
  for (int i = 0; i != _layersArray.size(); i++)
    _getLayers(&_layersArray[i], layerList, depth + 1);

  //
  return;
}

/*******************************************************************************/
/**
 **/
static std::vector<json11::Json> _getLayers(const json11::Json* map)
{
  std::vector<json11::Json> layerList;
  _getLayers(map, &layerList, 0);
  return layerList;
}

/*******************************************************************************/
/**
 * Load layers data from json11::Json to GameMap_t
 *
 * @return != 0 : error loading layers
 * @return 0 : layers loaded succesfully
 */
static int _loadMapLayers(const json11::Json* jsonMap, GameMap_t* map)
{
  std::vector<json11::Json> _layers;
  _layers = _getLayers(jsonMap);

  hash<string> hasher;

  //allocate memory for layers
  map->layersNum = _layers.size();
  map->layers = (GMapTilelayer_t*)calloc(_layers.size(), sizeof(GMapTilelayer_t));
  map->byteSize += (sizeof(GMapTilelayer_t) * _layers.size());
  //Fill layers with data
  for (int i = 0; i < _layers.size(); i++) {

    GMapTilelayer_t* _layer = &map->layers[i];
    //Check for unsupported encodings
    std::string _encoding = _layers[i]["encoding"].string_value();
    if (_encoding != "" && _encoding != "csv")
    {
      _GameMap_appendToErrStr("Layer \"" + _encoding + " " + _layers[i]["compression"].string_value() + "\" not supported\n");
      _GameMap_appendToErrStr("Save map as \"CSV\" and try again\n");
      return 1;
    }
    //layer name
    std::string _name = _layers[i]["name"].string_value();
    strncpy_s(_layer->name, _name.c_str(), _LAYER_NAME_MAXLEN);
    _layer->name[_LAYER_NAME_MAXLEN - 1] = '\0';
    //Name hash
    std::string _nameString(_layer->name);
    _layer->nameHash = hasher(_nameString);
    //
    _layer->width = _layers[i]["width"].int_value();
    _layer->height = _layers[i]["height"].int_value();
    _layer->offsetx = _layers[i]["offsetx"].int_value();
    _layer->offsety = _layers[i]["offsety"].int_value();
    _layer->opacity = _layers[i]["opacity"].number_value();
    _layer->visible = _layers[i]["visible"].bool_value() ? 1 : 0;
    //layer data
    size_t _dataLen = _layer->width * _layer->height;
    json11::Json::array _data = _layers[i]["data"].array_items();
    if (_dataLen != _data.size())
    {
      //ERROR expected size and data size does not match!
      _GameMap_appendToErrStr(
        "Map size is " + std::to_string(_layer->width)
        + "x" + std::to_string(_layer->height)
        + " = " + std::to_string(_dataLen)
        + " But layer data size is " + std::to_string(_data.size()) + " !!");
      return -1;
    }
    _layer->data = (unsigned int*)malloc(_dataLen * sizeof(unsigned int));
    map->byteSize += _dataLen * sizeof(unsigned int);
    for (int j = 0; j < _dataLen; j++)
    {//This is slow! there has to be another way to do this
      _layer->data[j] = _data[j].number_value();
    }
    //
  }
  return 0;
}

/*******************************************************************************/
static void _freeMapLayers(GameMap_t* map)
{
  if (map->layersNum == 0 || map->layers == nullptr)
    return;

  for (int i = 0; i < map->layersNum; i++) {
    GMapTilelayer_t* _layer = &map->layers[i];
    if(_layer->data != nullptr) free(_layer->data);
  }
  free(map->layers);
  map->layers = nullptr;
}

/*******************************************************************************/
/**
 * Load tilesets data from json11::Json to GameMap_t
 *
 * @return != 0 : error loading layers
 * @return 0 : layers loaded succesfully
 */
static int _loadMapTilesets(const json11::Json* jsonMap, GameMap_t* map, const char* baseDir)
{
  //allocate memory for tilesets
  if (true == (*jsonMap)["tilesets"].is_null())
    return 0;
  std::vector<json11::Json> _tilesets;
  _tilesets = (*jsonMap)["tilesets"].array_items();
  if (0 == _tilesets.size())
    return 0;

  map->tilesetsNum = _tilesets.size();
  map->tilesets = (GMapTileset_t*)calloc(_tilesets.size(), sizeof(GMapTileset_t));
  map->byteSize += (sizeof(GMapTileset_t) * _tilesets.size());
  //Fill tilesets with data
  for (int i = 0; i < _tilesets.size(); i++) {
    //
    GMapTileset_t* _tileset = &map->tilesets[i];
    //Check for external tilesets
    if (false == _tilesets[i]["source"].is_null()) {
      _GameMap_appendToErrStr(_tilesets[i]["source"].string_value());
      _GameMap_appendToErrStr(" External tilesets not supported\n");
      _GameMap_appendToErrStr("Fix:Open map with \"Tiled\" and set tileset as internal");
      return 1;
    }
    //tileset name
    std::string _name = _tilesets[i]["name"].string_value();
    strncpy_s(_tileset->name, _name.c_str(), _LAYER_NAME_MAXLEN);
    _tileset->name[_LAYER_NAME_MAXLEN - 1] = '\0';
    //tileset file path
    std::string _path = _tilesets[i]["image"].string_value();
    _path = baseDir + _path;
    strncpy_s(_tileset->imgPath, _path.c_str(), _LAYER_NAME_MAXLEN);
    _tileset->imgPath[_LAYER_NAME_MAXLEN - 1] = '\0';
    //
    _tileset->tilewidth = _tilesets[i]["tilewidth"].int_value();
    _tileset->tileheight = _tilesets[i]["tileheight"].int_value();
    _tileset->imagewidth = _tilesets[i]["imagewidth"].int_value();
    _tileset->imageheight = _tilesets[i]["imageheight"].int_value();
    _tileset->firstgid = _tilesets[i]["firstgid"].int_value();
    _tileset->tilecount = _tilesets[i]["tilecount"].int_value();
    //
  }
  return 0;
}

/*******************************************************************************/
static void _freeMapTilesets(GameMap_t* map)
{
  if (map->tilesetsNum == 0 || map->tilesets == nullptr)
    return;
  free(map->tilesets);
  map->tilesets = nullptr;
}

/*******************************************************************************/
void _GameMap_clearErrStr()
{
  _GameMap_errStr = "";
}

void _GameMap_appendToErrStr(const char* str)
{ _GameMap_errStr += str; }

void _GameMap_appendToErrStr(std::string str)
{ _GameMap_errStr += str; }

const char* GameMap_getErrStr()
{
  return _GameMap_errStr.c_str();
}

/*******************************************************************************/
static void _printTilemapData(const unsigned int *data, int w, int h)
{
  for(int i = 0 ; i < w*h ; i+=w)
  {
    printf("\t");
    for(int j = 0; j < w ; j++)
    {
      printf("%03u ",data[i+j]);
    }
    printf("\n");
  }
}
void GameMap_print(const GameMap_t *map)
{
  if (map == nullptr)
    return;
  printf("map\n"
        "\tsize : %u bytes\n"
         "\twidth:\t%d\n"
         "\theight:\t%d\n"
         "\ttileheight:\t%d\n"
         "\ttilewidth:\t%d\n",
         map->byteSize,
         map->width,
         map->height,
         map->tileheight,
         map->tilewidth);
  for(int i = 0; i != map->layersNum; i++){
    printf("\tlayer %d \"%s\"\n", i, map->layers[i].name);
    printf("\t\tWidth: %d\n\t\tHeight: %d\n",
    map->layers[i].width,map->layers[i].height);
    //_printTilemapData(map->layers[i].data,map->layers[i].width,map->layers[i].height);
  }
  for (int i = 0; i != map->tilesetsNum; i++) {
    printf("\ttileset %d \"%s\"\n", i, map->tilesets[i].name);
    printf("\t\tfile:\"%s\"\n\t\tWidth: %d\n\t\tHeight: %d\n"
           "\t\ttileWidth: %d \t\ttileHeight: %d\n"
           "\t\tfirstgid: %d\ttilecount: %d\n",
            map->tilesets[i].imgPath,map->tilesets[i].imagewidth, map->tilesets[i].imageheight,
            map->tilesets[i].tilewidth, map->tilesets[i].tileheight,
            map->tilesets[i].firstgid, map->tilesets[i].tilecount);
  }
}

