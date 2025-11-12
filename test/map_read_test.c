/*
 * This file is part of libtw07, a header-only library for teeworlds0.7.
 *
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
 */
#include "../lib/map.h"

int main(int argc, const char **argv)
{
    libtw07_enable_print = 1;

    libtw07_map_reader Reader;
    libtw07_map_reader_init(&Reader);
    libtw07_map_reader_open(&Reader, "test.map");
    if(libtw07_map_reader_isLoaded(&Reader) == -1)
        return -1;

    uint32_t Crc = libtw07_datafile_reader_crc(&Reader);
    SHA256_DIGEST Sha256 = libtw07_datafile_reader_sha256(&Reader);

    char aSha256[SHA256_MAXSTRSIZE];
    sha256_str(Sha256, aSha256, sizeof(aSha256));

    libtw07_print("test", "crc is %u, sha256 is %s", Crc, aSha256);

	int Start, Num;
	libtw07_datafile_reader_getType(&Reader, 0, &Start, &Num);
    int GroupsStart, GroupsNum, LayersStart, LayersNum;
    libtw07_datafile_reader_getType(&Reader, LIBTW07_MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
    libtw07_datafile_reader_getType(&Reader, LIBTW07_MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
    for(int g = 0; g < GroupsNum; g++)
    {
        libtw07_map_itemGroup *pGroup = (libtw07_map_itemGroup *) libtw07_datafile_reader_getItem(&Reader, GroupsStart + g, 0, 0);

        char aGroupName[sizeof(int) * 3] = {'(', 'u', 'n', 'k', 'n', 'o', 'w', 'n', ')', '\0'};
        if(pGroup->m_Version >= 3)
            libtw07_intsToStr(pGroup->m_aName, 3, aGroupName);
        if(!aGroupName[0])
            strncpy(aGroupName, "(unnamed)", sizeof(aGroupName));
        libtw07_print("test", "> group: %s", aGroupName);
        for(int l = 0; l < pGroup->m_NumLayers; l++)
        {
            libtw07_map_itemLayer *pLayer = (libtw07_map_itemLayer *) libtw07_datafile_reader_getItem(&Reader, LayersStart + pGroup->m_StartLayer + l, 0, 0);
            if(pLayer->m_Type == LIBTW07_LAYERTYPE_TILES)
            {
                libtw07_map_itemLayerTilemap *pTilemap = (libtw07_map_itemLayerTilemap *) pLayer;

                char aLayerName[sizeof(int) * 3] = {'(', 'u', 'n', 'k', 'n', 'o', 'w', 'n', ')', '\0'};
                if(pTilemap->m_Version >= 3)
                    libtw07_intsToStr(pTilemap->m_aName, 3, aLayerName);
                if(!aLayerName[0])
                    strncpy(aLayerName, "(unnamed)", sizeof(aLayerName));
                libtw07_print("test", "| %stilemap layer: %s", (pTilemap->m_Flags&LIBTW07_TILESLAYERFLAG_GAME ? "game " : ""), aLayerName);
            }
            else if(pLayer->m_Type == LIBTW07_LAYERTYPE_QUADS)
            {
                libtw07_map_itemLayerQuads *pLayerQuads = (libtw07_map_itemLayerQuads *) pLayer;
                char aLayerName[sizeof(int) * 3] = {'(', 'u', 'n', 'k', 'n', 'o', 'w', 'n', ')', '\0'};
				if(pLayerQuads->m_Version >= 2)
                    libtw07_intsToStr(pLayerQuads->m_aName, 3, aLayerName);
                if(!aLayerName[0])
                    strncpy(aLayerName, "(unnamed)", sizeof(aLayerName));
                libtw07_print("test", "| quads layer: %s", aLayerName);
            }
        }
    }

    libtw07_map_reader_unload(&Reader);
    return 0;
}
