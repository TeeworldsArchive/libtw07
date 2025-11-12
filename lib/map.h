/*
 * This file is part of libtw07, a header-only library for teeworlds0.7.
 *
 * Orignal Code by:
 * Copyright (C) 2007-2025 Magnus Auvinen
 *
 * Libtw07 Library:
 * Copyright (C) 2025 TeeworldsArchive
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Note: This is an altered version — a C language port of the original C++ implementation.
 */
#include "datafile.h"

typedef libtw07_datafileReader libtw07_map_reader;

// layer types
enum
{
	LIBTW07_LAYERTYPE_INVALID = 0,
	LIBTW07_LAYERTYPE_GAME,
	LIBTW07_LAYERTYPE_TILES,
	LIBTW07_LAYERTYPE_QUADS,

	LIBTW07_MAPITEMTYPE_VERSION = 0,
	LIBTW07_MAPITEMTYPE_INFO,
	LIBTW07_MAPITEMTYPE_IMAGE,
	LIBTW07_MAPITEMTYPE_ENVELOPE,
	LIBTW07_MAPITEMTYPE_GROUP,
	LIBTW07_MAPITEMTYPE_LAYER,
	LIBTW07_MAPITEMTYPE_ENVPOINTS,

	LIBTW07_CURVETYPE_STEP = 0,
	LIBTW07_CURVETYPE_LINEAR,
	LIBTW07_CURVETYPE_SLOW,
	LIBTW07_CURVETYPE_FAST,
	LIBTW07_CURVETYPE_SMOOTH,
	LIBTW07_CURVETYPE_BEZIER,
	LIBTW07_NUM_CURVETYPES,

	// game layer tiles
	LIBTW07_ENTITY_NULL = 0,
	LIBTW07_ENTITY_SPAWN,
	LIBTW07_ENTITY_SPAWN_RED,
	LIBTW07_ENTITY_SPAWN_BLUE,
	LIBTW07_ENTITY_FLAGSTAND_RED,
	LIBTW07_ENTITY_FLAGSTAND_BLUE,
	LIBTW07_ENTITY_ARMOR,
	LIBTW07_ENTITY_HEALTH,
	LIBTW07_ENTITY_WEAPON_SHOTGUN,
	LIBTW07_ENTITY_WEAPON_GRENADE,
	LIBTW07_ENTITY_POWERUP_NINJA,
	LIBTW07_ENTITY_WEAPON_LASER,
	LIBTW07_NUM_ENTITIES,

	LIBTW07_TILE_AIR = 0,
	LIBTW07_TILE_SOLID,
	LIBTW07_TILE_DEATH,
	LIBTW07_TILE_NOHOOK,

	LIBTW07_TILEFLAG_VFLIP = 1,
	LIBTW07_TILEFLAG_HFLIP = 2,
	LIBTW07_TILEFLAG_OPAQUE = 4,
	LIBTW07_TILEFLAG_ROTATE = 8,

	LIBTW07_LAYERFLAG_DETAIL = 1,
	LIBTW07_TILESLAYERFLAG_GAME = 1,

	LIBTW07_ENTITY_OFFSET = 255 - 16 * 4,
};

struct libtw07_map_point
{
	int x, y; // 22.10 fixed point
};
typedef struct libtw07_map_point libtw07_map_point;

struct libtw07_map_color
{
	int r, g, b, a;
};
typedef struct libtw07_map_color libtw07_map_color;

struct libtw07_map_quad
{
	libtw07_map_point m_aPoints[5];
	libtw07_map_color m_aColors[4];
	libtw07_map_point m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};
typedef struct libtw07_map_quad libtw07_map_quad;

struct libtw07_map_tile
{
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};
typedef struct libtw07_map_tile libtw07_map_tile;

struct libtw07_map_itemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
};
#define LIBTW07_MAP_ITEMINFO_CURRENT_VERSION 1
typedef struct libtw07_map_itemInfo libtw07_map_itemInfo;

struct libtw07_map_itemImage_v1
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
};
typedef struct libtw07_map_itemImage_v1 libtw07_map_itemImage_v1;

struct libtw07_map_itemImage
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
	int m_MustBe1;
};
#define LIBTW07_MAP_ITEMIMAGE_CURRENT_VERSION 2
typedef struct libtw07_map_itemImage libtw07_map_itemImage;

struct libtw07_map_itemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
};
typedef struct libtw07_map_itemGroup_v1 libtw07_map_itemGroup_v1;

struct libtw07_map_itemGroup
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
};
#define LIBTW07_MAP_ITEMGROUP_CURRENT_VERSION 3
typedef struct libtw07_map_itemGroup libtw07_map_itemGroup;

struct libtw07_map_itemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
};
typedef struct libtw07_map_itemLayer libtw07_map_itemLayer;

struct libtw07_map_itemLayerTilemap
{
	libtw07_map_itemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	libtw07_map_color m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;

	int m_aName[3];
};
#define LIBTW07_MAP_ITEMLAYERTILEMAP_CURRENT_VERSION 4
typedef struct libtw07_map_itemLayerTilemap libtw07_map_itemLayerTilemap;

struct libtw07_map_itemLayerQuads
{
	libtw07_map_itemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
};
#define LIBTW07_MAP_ITEMLAYERQUADS_CURRENT_VERSION 2
typedef struct libtw07_map_itemLayerQuads libtw07_map_itemLayerQuads;

struct libtw07_map_itemVersion
{
	int m_Version;
};
#define LIBTW07_MAP_ITEMVERSION_CURRENT_VERSION 1
typedef struct libtw07_map_itemVersion libtw07_map_itemVersion;

struct libtw07_map_envPoint_v1
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)
};
typedef struct libtw07_map_envPoint_v1 libtw07_map_envPoint_v1;

struct libtw07_map_envPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)
	// bezier curve only
	// dx in ms and dy as 22.10 fxp
	int m_aInTangentdx[4];
	int m_aInTangentdy[4];
	int m_aOutTangentdx[4];
	int m_aOutTangentdy[4];
};
typedef struct libtw07_map_envPoint libtw07_map_envPoint;

struct libtw07_map_itemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
};
typedef struct libtw07_map_itemEnvelope_v1 libtw07_map_itemEnvelope_v1;

struct libtw07_map_itemEnvelope
{
	// version 3 = bezier curve support
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
    // version 2
	int m_Synchronized;
};
#define LIBTW07_MAP_ITEMENVELOPE_CURRENT_VERSION 3
typedef struct libtw07_map_itemEnvelope libtw07_map_itemEnvelope;

void libtw07_strToInts(int *pInts, int Num, const char *pStr)
{
	int Index = 0;
	while(Num)
	{
		char aBuf[4] = {0,0,0,0};
		for(int c = 0; c < 4 && pStr[Index]; c++, Index++)
			aBuf[c] = pStr[Index];
		*pInts = ((aBuf[0]+128)<<24)|((aBuf[1]+128)<<16)|((aBuf[2]+128)<<8)|(aBuf[3]+128);
		pInts++;
		Num--;
	}

	// null terminate
	pInts[-1] &= 0xffffff00;
}

void libtw07_intsToStr(const int *pInts, int Num, char *pStr)
{
	while(Num)
	{
		pStr[0] = (((*pInts)>>24)&0xff)-128;
		pStr[1] = (((*pInts)>>16)&0xff)-128;
		pStr[2] = (((*pInts)>>8)&0xff)-128;
		pStr[3] = ((*pInts)&0xff)-128;
		pStr += 4;
		pInts++;
		Num--;
	}

	// null terminate
	pStr[-1] = 0;
}

int libtw07_map_reader_open(libtw07_map_reader *pMap, const char *pMapPath)
{
    if(!libtw07_datafile_reader_open(pMap, pMapPath))
        return -1;
    // check version
    libtw07_map_itemVersion *pItem = (libtw07_map_itemVersion *) libtw07_datafile_reader_findItem(pMap, LIBTW07_MAPITEMTYPE_VERSION, 0);
    if(!pItem || pItem->m_Version != LIBTW07_MAP_ITEMVERSION_CURRENT_VERSION)
        return -1;

    // replace compressed tile layers with uncompressed ones
    int GroupsStart, GroupsNum, LayersStart, LayersNum;
    libtw07_datafile_reader_getType(pMap, LIBTW07_MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
    libtw07_datafile_reader_getType(pMap, LIBTW07_MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
    for(int g = 0; g < GroupsNum; g++)
    {
        libtw07_map_itemGroup *pGroup = (libtw07_map_itemGroup *) libtw07_datafile_reader_getItem(pMap, GroupsStart + g, 0, 0);
        for(int l = 0; l < pGroup->m_NumLayers; l++)
        {
            libtw07_map_itemLayer *pLayer = (libtw07_map_itemLayer *) libtw07_datafile_reader_getItem(pMap, LayersStart + pGroup->m_StartLayer + l, 0, 0);
            if(pLayer->m_Type == LIBTW07_LAYERTYPE_TILES)
            {
                libtw07_map_itemLayerTilemap *pTilemap = (libtw07_map_itemLayerTilemap *) pLayer;

                if(pTilemap->m_Version > 3)
                {
                    const int TilemapCount = pTilemap->m_Width * pTilemap->m_Height;
                    const int TilemapSize = TilemapCount * sizeof(libtw07_map_tile);

                    if((TilemapCount / pTilemap->m_Width != pTilemap->m_Height) || (TilemapSize / (int) sizeof(libtw07_map_tile) != TilemapCount))
                    {
                        libtw07_print("map", "map layer too big (%d * %d * %u causes an integer overflow)", pTilemap->m_Width, pTilemap->m_Height, (unsigned)(sizeof(libtw07_map_tile)));
                        return -1;
                    }
                    libtw07_map_tile *pTiles = (libtw07_map_tile *) malloc(TilemapSize);
                    if(!pTiles)
                        return -1;

                    // extract original tile data
                    int i = 0;
                    libtw07_map_tile *pSavedTiles = (libtw07_map_tile *) libtw07_datafile_reader_getData(pMap, pTilemap->m_Data);
                    while(i < TilemapCount)
                    {
                        for(unsigned Counter = 0; Counter <= pSavedTiles->m_Skip && i < TilemapCount; Counter++)
                        {
                            pTiles[i] = *pSavedTiles;
                            pTiles[i++].m_Skip = 0;
                        }

                        pSavedTiles++;
                    }

                    libtw07_datafile_reader_replaceData(pMap, pTilemap->m_Data, (char *) pTiles, TilemapSize);
                }
            }
        }
    }

    return 0;
}

void libtw07_map_reader_init(libtw07_map_reader *pMap)
{
	libtw07_datafile_reader_init(pMap);
}

int libtw07_map_reader_isLoaded(libtw07_map_reader *pMap)
{
	return libtw07_datafile_reader_isOpen(pMap);
}

void libtw07_map_reader_unload(libtw07_map_reader *pMap)
{
	return libtw07_datafile_reader_destroy(pMap);
}
