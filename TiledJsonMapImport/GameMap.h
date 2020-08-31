/*******************************************************************************
 * GameMap
 * Public header
*******************************************************************************/

#ifndef _GAME_MAP_H
#define _GAME_MAP_H

typedef struct GameMap_t GameMap_t;

/**
 * Load map from Tiled JSON file, on load error return nullptr 
 * 
 * @return nullptr : error loading file
 * @return != nullptr : pointer to new  map
 */
GameMap_t *GameMap_loadFromTiledJSON(const char *path);

/**
* delete game map
* free game map memory
*/
void GameMap_free(GameMap_t* map);

/**
* Draw GameMap with camera position at tile (x,y)
*/
void DrawGameMap(const GameMap_t* map, double x, double y);

/**
* Get tile ID of tile (x,y) in layer "layername"
* Example: GameMap_getLayerTileID("collision_map",140,300);
* @return ID of tile
* @return 0 if wrong layerName or (tx,ty)
*/
unsigned int GameMap_getTileId(const GameMap_t* map, const char* layerName, int tx, int ty);
unsigned int GameMap_getTileId(const GameMap_t* map, int layerId, int tx, int ty);

/**
* Get error string
* When GameMap_loadFromTiledJSON(), LoadGameMapGraphs() or similar fails
* Descriptive error messages will appear in this string
*/
const char* GameMap_getErrStr();

/**
* Print game map debug information into stdout
*/
void GameMap_print(const GameMap_t *map);


#endif
