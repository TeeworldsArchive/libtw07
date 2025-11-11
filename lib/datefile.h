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
#ifndef LIBTW07_DATAFILE_H
#define LIBTW07_DATAFILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "external/miniz/miniz.h"

#include "detect.h"
#include "hash.h"
#include "math.h"
#include "print.h"


#ifdef __cplusplus
extern "C" {
#endif

static const int DEBUG=0;

struct _libtw07_datafileItemType
{
	int m_Type;
	int m_Start;
	int m_Num;
};
typedef struct _libtw07_datafileItemType libtw07_datafileItemType;

struct _libtw07_datafileItem
{
	int m_TypeAndID;
	int m_Size;
};
typedef struct _libtw07_datafileItem libtw07_datafileItem;

struct _libtw07_datafileHeader
{
	char m_aID[4];
	int m_Version;
	int m_Size;
	int m_Swaplen;
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
};
typedef struct _libtw07_datafileHeader libtw07_datafileHeader;

struct _libtw07_datafileData
{
	int m_NumItemTypes;
	int m_NumItems;
	int m_NumRawData;
	int m_ItemSize;
	int m_DataSize;
	char m_aStart[4];
};
typedef struct _libtw07_datafileData libtw07_datafileData;

struct _libtw07_datafileInfo
{
	libtw07_datafileItemType *m_pItemTypes;
	int *m_pItemOffsets;
	int *m_pDataOffsets;
	int *m_pDataSizes;

	char *m_pItemStart;
	char *m_pDataStart;
};
typedef struct _libtw07_datafileInfo libtw07_datafileInfo;

struct _libtw07_datafile
{
	FILE *m_File;
	SHA256_DIGEST m_Sha256;
	uint32_t m_Crc;
	libtw07_datafileInfo m_Info;
	libtw07_datafileHeader m_Header;
	int m_DataStartOffset;
	char **m_ppDataPtrs;
	int *m_pDataSizes;
	char *m_pData;
};
typedef struct _libtw07_datafile libtw07_datafile;

struct _libtw07_datafileReader
{
	libtw07_datafile *m_pDataFile;
};
typedef struct _libtw07_datafileReader libtw07_datafileReader;

int libtw07_datafile_reader_open(libtw07_datafileReader *pReader, const char *pFilename);
int libtw07_datafile_reader_close(libtw07_datafileReader *pReader);

void libtw07_datafile_reader_init(libtw07_datafileReader *pReader)
{
	pReader->m_pDataFile = NULL;
}

void libtw07_datafile_reader_destroy(libtw07_datafileReader *pReader)
{
	libtw07_datafile_reader_close(pReader);
}

int libtw07_datafile_reader_open(libtw07_datafileReader *pReader, const char *pFilename)
{
	libtw07_print("datafile", "loading. filename='%s'", pFilename);

	FILE *File = fopen(pFilename, "rb");
	if(!File)
	{
		libtw07_print("datafile", "could not open '%s'", pFilename);
		return -1;
	}


	// take the hashes of the file and store them
	SHA256_CTX Sha256Ctx;
	sha256_init(&Sha256Ctx);
	uint32_t Crc = crc32(0L, 0x0, 0);
	{
		enum
		{
			BUFFER_SIZE = 64*1024
		};

		unsigned char aBuffer[BUFFER_SIZE];

		while(1)
		{
			unsigned Bytes = fread(aBuffer, 1, BUFFER_SIZE, File);
			if(Bytes == 0)
				break;
			sha256_update(&Sha256Ctx, aBuffer, Bytes);
			Crc = crc32(Crc, aBuffer, Bytes);
		}

		fseek(File, 0, SEEK_SET);
	}

	libtw07_datafileHeader Header;
	fread(&Header, 1, sizeof(Header), File);
	if(Header.m_aID[0] != 'A' || Header.m_aID[1] != 'T' || Header.m_aID[2] != 'A' || Header.m_aID[3] != 'D')
	{
		if(Header.m_aID[0] != 'D' || Header.m_aID[1] != 'A' || Header.m_aID[2] != 'T' || Header.m_aID[3] != 'A')
		{
			libtw07_print("datafile", "wrong signature. %x %x %x %x", Header.m_aID[0], Header.m_aID[1], Header.m_aID[2], Header.m_aID[3]);
			fclose(File);
			return 0;
		}
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif
	if(Header.m_Version != 3 && Header.m_Version != 4)
	{
		libtw07_print("datafile", "wrong version. version=%x", Header.m_Version);
		fclose(File);
		return 0;
	}

	// read in the rest except the data
	int64_t Size = 0;
	Size += Header.m_NumItemTypes*sizeof(libtw07_datafileItemType);
	Size += (Header.m_NumItems+Header.m_NumRawData)*sizeof(int);
	if(Header.m_Version == 4)
		Size += Header.m_NumRawData*sizeof(int); // v4 has uncompressed data sizes aswell
	Size += Header.m_ItemSize;

	int64_t AllocSize = Size;
	AllocSize += sizeof(libtw07_datafile); // add space for info structure
	AllocSize += Header.m_NumRawData*sizeof(void*); // add space for data pointers
	AllocSize += Header.m_NumRawData*sizeof(int); // add space for data sizes
	if(Size > (1LL << 31LL) || Header.m_NumItemTypes < 0 || Header.m_NumItems < 0 || Header.m_NumRawData < 0 || Header.m_ItemSize < 0)
	{
		fclose(File);
		libtw07_print("datafile", "unable to load file, invalid file information");
		return -1;
	}

	libtw07_datafile *pTmpDataFile = (libtw07_datafile *) malloc(AllocSize);
	pTmpDataFile->m_Header = Header;
	pTmpDataFile->m_DataStartOffset = sizeof(libtw07_datafileHeader) + Size;
	pTmpDataFile->m_ppDataPtrs = (char **)(pTmpDataFile+1);
	pTmpDataFile->m_pDataSizes = (int *)(pTmpDataFile->m_ppDataPtrs + Header.m_NumRawData);
	pTmpDataFile->m_pData = (char *)(pTmpDataFile->m_pDataSizes + Header.m_NumRawData);
	pTmpDataFile->m_File = File;
	pTmpDataFile->m_Sha256 = sha256_finish(&Sha256Ctx);
	pTmpDataFile->m_Crc = Crc;

	// clear the data pointers and sizes
	memset(pTmpDataFile->m_ppDataPtrs, 0, Header.m_NumRawData*sizeof(void*));
	memset(pTmpDataFile->m_pDataSizes, 0, Header.m_NumRawData*sizeof(int));

	// read types, offsets, sizes and item data
	uint32_t ReadSize = fread(pTmpDataFile->m_pData, 1, Size, File);
	if(ReadSize != Size)
	{
		fclose(pTmpDataFile->m_File);
		free(pTmpDataFile);
		pTmpDataFile = 0;
		libtw07_print("datafile", "couldn't load the whole thing, wanted=%d got=%d", (uint32_t) Size, ReadSize);
		return -1;
	}

	libtw07_datafile_reader_close(pReader);
	pReader->m_pDataFile = pTmpDataFile;

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(pReader->m_pDataFile->m_pData, sizeof(int), libtw07_minimum((uint32_t) Header.m_Swaplen, (uint32_t) Size) / sizeof(int));
#endif

	//if(DEBUG)
	{
		libtw07_print("datafile", "allocsize=%d", (uint32_t) AllocSize);
		libtw07_print("datafile", "readsize=%d", ReadSize);
		libtw07_print("datafile", "swaplen=%d", Header.m_Swaplen);
		libtw07_print("datafile", "item_size=%d", pReader->m_pDataFile->m_Header.m_ItemSize);
	}

	pReader->m_pDataFile->m_Info.m_pItemTypes = (libtw07_datafileItemType *)pReader->m_pDataFile->m_pData;
	pReader->m_pDataFile->m_Info.m_pItemOffsets = (int *)&pReader->m_pDataFile->m_Info.m_pItemTypes[pReader->m_pDataFile->m_Header.m_NumItemTypes];
	pReader->m_pDataFile->m_Info.m_pDataOffsets = (int *)&pReader->m_pDataFile->m_Info.m_pItemOffsets[pReader->m_pDataFile->m_Header.m_NumItems];
	pReader->m_pDataFile->m_Info.m_pDataSizes = (int *)&pReader->m_pDataFile->m_Info.m_pDataOffsets[pReader->m_pDataFile->m_Header.m_NumRawData];

	if(Header.m_Version == 4)
		pReader->m_pDataFile->m_Info.m_pItemStart = (char *)&pReader->m_pDataFile->m_Info.m_pDataSizes[pReader->m_pDataFile->m_Header.m_NumRawData];
	else
		pReader->m_pDataFile->m_Info.m_pItemStart = (char *)&pReader->m_pDataFile->m_Info.m_pDataOffsets[pReader->m_pDataFile->m_Header.m_NumRawData];
	pReader->m_pDataFile->m_Info.m_pDataStart = pReader->m_pDataFile->m_Info.m_pItemStart + pReader->m_pDataFile->m_Header.m_ItemSize;

	libtw07_print("datafile", "loading done. datafile='%s'", pFilename);

	return 0;
}

int libtw07_datafile_numData(libtw07_datafileReader *pReader)
{
	if(!pReader->m_pDataFile) { return 0; }
	return pReader->m_pDataFile->m_Header.m_NumRawData;
}

int _libtw07_datafile_getFileDataSize(libtw07_datafileReader *pReader, int Index)
{
	if(!pReader->m_pDataFile) { return 0; }

	if(Index == pReader->m_pDataFile->m_Header.m_NumRawData - 1)
		return pReader->m_pDataFile->m_Header.m_DataSize - pReader->m_pDataFile->m_Info.m_pDataOffsets[Index];
	return pReader->m_pDataFile->m_Info.m_pDataOffsets[Index + 1] - pReader->m_pDataFile->m_Info.m_pDataOffsets[Index];
}

int libtw07_datafile_getDataSize(libtw07_datafileReader *pReader, int Index)
{
	if(Index < 0 || Index >= pReader->m_pDataFile->m_Header.m_NumRawData)
	{
		return 0;
	}

	if(!pReader->m_pDataFile->m_ppDataPtrs[Index])
	{
		if(pReader->m_pDataFile->m_Header.m_Version >= 4)
		{
			return pReader->m_pDataFile->m_Info.m_pDataSizes[Index];
		}
		else
		{
			return _libtw07_datafile_getFileDataSize(pReader, Index);
		}
	}
	return pReader->m_pDataFile->m_pDataSizes[Index];
}

void *_libtw07_datafile_getDataImpl(libtw07_datafileReader *pReader, int Index, int Swap)
{
	if(!pReader->m_pDataFile) { return 0; }

	if(Index < 0 || Index >= pReader->m_pDataFile->m_Header.m_NumRawData)
		return 0;

	// load it if needed
	if(!pReader->m_pDataFile->m_ppDataPtrs[Index])
	{
		// fetch the data size
		int DataSize = libtw07_datafile_getDataSize(pReader, Index);
#if defined(CONF_ARCH_ENDIAN_BIG)
		int SwapSize = DataSize;
#endif

		if(pReader->m_pDataFile->m_Header.m_Version == 4)
		{
			// v4 has compressed data
			void *pTemp = malloc(DataSize);
			unsigned long UncompressedSize = pReader->m_pDataFile->m_Info.m_pDataSizes[Index];
			unsigned long s;

			libtw07_print("datafile", "loading data index=%d size=%d uncompressed=%lu", Index, DataSize, UncompressedSize);
			pReader->m_pDataFile->m_ppDataPtrs[Index] = (char *) malloc(UncompressedSize);
			pReader->m_pDataFile->m_pDataSizes[Index] = UncompressedSize;

			// read the compressed data
			fseek(pReader->m_pDataFile->m_File, pReader->m_pDataFile->m_DataStartOffset+pReader->m_pDataFile->m_Info.m_pDataOffsets[Index], SEEK_SET);
			fread(pTemp, 1, DataSize, pReader->m_pDataFile->m_File);

			// decompress the data, TODO: check for errors
			s = UncompressedSize;
			uncompress((Bytef*)pReader->m_pDataFile->m_ppDataPtrs[Index], &s, (Bytef*)pTemp, DataSize);
#if defined(CONF_ARCH_ENDIAN_BIG)
			SwapSize = s;
#endif

			// clean up the temporary buffers
			free(pTemp);
		}
		else
		{
			// load the data
			libtw07_print("datafile", "loading data index=%d size=%d", Index, DataSize);
			pReader->m_pDataFile->m_ppDataPtrs[Index] = (char *) malloc(DataSize);
			pReader->m_pDataFile->m_pDataSizes[Index] = DataSize;
			fseek(pReader->m_pDataFile->m_File, pReader->m_pDataFile->m_DataStartOffset + pReader->m_pDataFile->m_Info.m_pDataOffsets[Index], SEEK_SET);
			fread(pReader->m_pDataFile->m_ppDataPtrs[Index], 1, DataSize, pReader->m_pDataFile->m_File);
		}

#if defined(CONF_ARCH_ENDIAN_BIG)
		if(Swap && SwapSize)
			swap_endian(pReader->m_pDataFile->m_ppDataPtrs[Index], sizeof(int), SwapSize/sizeof(int));
#endif
	}

	return pReader->m_pDataFile->m_ppDataPtrs[Index];
}

void *libtw07_datafile_getData(libtw07_datafileReader *pReader, int Index)
{
	return _libtw07_datafile_getDataImpl(pReader, Index, 0);
}

void *libtw07_datafile_getDataSwapped(libtw07_datafileReader *pReader, int Index)
{
	return _libtw07_datafile_getDataImpl(pReader, Index, 1);
}

void libtw07_datafile_unloadData(libtw07_datafileReader *pReader,int Index)
{
	if(Index < 0 || Index >= pReader->m_pDataFile->m_Header.m_NumRawData)
		return;

	free(pReader->m_pDataFile->m_ppDataPtrs[Index]);
	pReader->m_pDataFile->m_ppDataPtrs[Index] = 0x0;
	pReader->m_pDataFile->m_pDataSizes[Index] = 0;
}

void libtw07_datafile_replaceData(libtw07_datafileReader *pReader, int Index, char *pData, int Size)
{
	if(Index < 0 || Index >= pReader->m_pDataFile->m_Header.m_NumRawData)
		return;

	// make sure the data has been loaded
	_libtw07_datafile_getDataImpl(pReader, Index, 0);

	libtw07_datafile_unloadData(pReader, Index);
	pReader->m_pDataFile->m_ppDataPtrs[Index] = pData;
	pReader->m_pDataFile->m_pDataSizes[Index] = Size;
}

int _libtw07_datafile_getFileItemSize(libtw07_datafileReader *pReader, int Index)
{
	if(!pReader->m_pDataFile) { return 0; }
	if(Index == pReader->m_pDataFile->m_Header.m_NumItems-1)
		return pReader->m_pDataFile->m_Header.m_ItemSize - pReader->m_pDataFile->m_Info.m_pItemOffsets[Index];
	return pReader->m_pDataFile->m_Info.m_pItemOffsets[Index+1] - pReader->m_pDataFile->m_Info.m_pItemOffsets[Index];
}

int libtw07_datafile_getItemSize(libtw07_datafileReader *pReader, int Index)
{
	int FileSize = _libtw07_datafile_getFileItemSize(pReader, Index);
	if(FileSize == 0)
	{
		return 0;
	}
	return FileSize - sizeof(libtw07_datafileItem);
}

void *libtw07_datafile_getItem(libtw07_datafileReader *pReader, int Index, int *pType, int *pID)
{
	if(!pReader->m_pDataFile || Index < 0 || Index >= pReader->m_pDataFile->m_Header.m_NumItems)
	{
		if(pType)
			*pType = 0;
		if(pID)
			*pID = 0;

		return 0;
	}

	libtw07_datafileItem *i = (libtw07_datafileItem *) pReader->m_pDataFile->m_Info.m_pItemStart + pReader->m_pDataFile->m_Info.m_pItemOffsets[Index];
	if(pType)
		*pType = (i->m_TypeAndID>>16)&0xffff; // remove sign extention
	if(pID)
		*pID = i->m_TypeAndID&0xffff;
	return (void *)(i+1);
}

void libtw07_datafile_getType(libtw07_datafileReader *pReader, int Type, int *pStart, int *pNum)
{
	*pStart = 0;
	*pNum = 0;

	if(!pReader->m_pDataFile)
		return;

	for(int i = 0; i < pReader->m_pDataFile->m_Header.m_NumItemTypes; i++)
	{
		if(pReader->m_pDataFile->m_Info.m_pItemTypes[i].m_Type == Type)
		{
			*pStart = pReader->m_pDataFile->m_Info.m_pItemTypes[i].m_Start;
			*pNum = pReader->m_pDataFile->m_Info.m_pItemTypes[i].m_Num;
			return;
		}
	}
}

void *libtw07_datafile_findItem(libtw07_datafileReader *pReader, int Type, int ID)
{
	if(!pReader->m_pDataFile) return 0;

	int Start, Num;
	libtw07_datafile_getType(pReader, Type, &Start, &Num);
	for(int i = 0; i < Num; i++)
	{
		int ItemID;
		void *pItem = libtw07_datafile_getItem(pReader, Start+i,0, &ItemID);
		if(ID == ItemID)
			return pItem;
	}
	return 0;
}

int libtw07_datafile_numItems(libtw07_datafileReader *pReader)
{
	if(!pReader->m_pDataFile) return 0;
	return pReader->m_pDataFile->m_Header.m_NumItems;
}

int libtw07_datafile_reader_close(libtw07_datafileReader *pReader)
{
	if(!pReader->m_pDataFile)
		return 0;

	// free the data that is loaded
	int i;
	for(i = 0; i < pReader->m_pDataFile->m_Header.m_NumRawData; i++)
	{
		free(pReader->m_pDataFile->m_ppDataPtrs[i]);
		pReader->m_pDataFile->m_pDataSizes[i] = 0;
	}

	fclose(pReader->m_pDataFile->m_File);
	free(pReader->m_pDataFile);
	pReader->m_pDataFile = 0;
	return 0;
}

SHA256_DIGEST libtw07_datafile_sha256(libtw07_datafileReader *pReader)
{
	if(!pReader->m_pDataFile) return SHA256_ZEROED;
	return pReader->m_pDataFile->m_Sha256;
}

uint32_t libtw07_datafile_crc(libtw07_datafileReader *pReader)
{
	if(!pReader->m_pDataFile) return 0xFFFFFFFF;
	return pReader->m_pDataFile->m_Crc;
}

int libtw07_datafile_checkSha256(FILE *File, const void *pSha256)
{
	// read the hash of the file
	SHA256_CTX Sha256Ctx;
	sha256_init(&Sha256Ctx);
	unsigned char aBuffer[64*1024];
	
	while(1)
	{
		unsigned Bytes = fread(aBuffer, 1, sizeof(aBuffer), File);
		if(Bytes == 0)
			break;
		sha256_update(&Sha256Ctx, aBuffer, Bytes);
	}

	fseek(File, 0, SEEK_SET);
	SHA256_DIGEST Sha256 = sha256_finish(&Sha256Ctx);

	return !sha256_comp(*(const SHA256_DIGEST *)pSha256, Sha256);
}

struct _libtw07_datafileWriter_dataInfo
{
	int m_UncompressedSize;
	int m_CompressedSize;
	void *m_pCompressedData;
};
typedef struct _libtw07_datafileWriter_dataInfo libtw07_datafileWriter_dataInfo;

struct _libtw07_datafileWriter_itemInfo
{
	int m_Type;
	int m_ID;
	int m_Size;
	int m_Next;
	int m_Prev;
	void *m_pData;
};
typedef struct _libtw07_datafileWriter_itemInfo libtw07_datafileWriter_itemInfo;

struct _libtw07_datafileWriter_itemTypeInfo
{
	int m_Num;
	int m_First;
	int m_Last;
};
typedef struct _libtw07_datafileWriter_itemTypeInfo libtw07_datafileWriter_itemTypeInfo;

struct _libtw07_datafileWriter
{
	FILE *m_File;
	int m_NumItems;
	int m_NumDatas;
	int m_NumItemTypes;
	libtw07_datafileWriter_itemTypeInfo *m_pItemTypes;
	libtw07_datafileWriter_itemInfo *m_pItems;
	libtw07_datafileWriter_dataInfo *m_pDatas;
};
typedef struct _libtw07_datafileWriter libtw07_datafileWriter;

enum
{
	LIBTW07_DATAFILE_MAX_ITEM_TYPES=0xffff,
	LIBTW07_DATAFILE_MAX_ITEMS=1024,
	LIBTW07_DATAFILE_MAX_DATAS=1024,
};

void libtw07_datafile_writer_init(libtw07_datafileWriter *pWriter)
{
	pWriter->m_File = 0;
	pWriter->m_pItemTypes = (libtw07_datafileWriter_itemTypeInfo *) malloc(sizeof(libtw07_datafileWriter_itemTypeInfo) * LIBTW07_DATAFILE_MAX_ITEM_TYPES);
	pWriter->m_pItems = (libtw07_datafileWriter_itemInfo *) malloc(sizeof(libtw07_datafileWriter_itemInfo) * LIBTW07_DATAFILE_MAX_ITEMS);
	pWriter->m_pDatas = (libtw07_datafileWriter_dataInfo *) malloc(sizeof(libtw07_datafileWriter_dataInfo) * LIBTW07_DATAFILE_MAX_DATAS);
}

void libtw07_datafile_writer_destroy(libtw07_datafileWriter *pWriter)
{
	free(pWriter->m_pItemTypes);
	pWriter->m_pItemTypes = 0;
	free(pWriter->m_pItems);
	pWriter->m_pItems = 0;
	free(pWriter->m_pDatas);
	pWriter->m_pDatas = 0;
}

int libtw07_datafile_writer_open(libtw07_datafileWriter *pWriter, const char *pFilename)
{
	libtw07_dbg_assert(!pWriter->m_File, "a file already exists");
	pWriter->m_File = fopen(pFilename, "wb");
	if(!pWriter->m_File)
		return -1;

	pWriter->m_NumItems = 0;
	pWriter->m_NumDatas = 0;
	pWriter->m_NumItemTypes = 0;
	memset(pWriter->m_pItemTypes, 0, sizeof(libtw07_datafileWriter_itemTypeInfo) * LIBTW07_DATAFILE_MAX_ITEM_TYPES);

	for(int i = 0; i < LIBTW07_DATAFILE_MAX_ITEM_TYPES; i++)
	{
		pWriter->m_pItemTypes[i].m_First = -1;
		pWriter->m_pItemTypes[i].m_Last = -1;
	}

	return 0;
}

int libtw07_datafile_addItem(libtw07_datafileWriter *pWriter, int Type, int ID, int Size, const void *pData)
{
	if(!pWriter->m_File) return 0;

	libtw07_dbg_assert(Type >= 0 && Type < 0xFFFF, "incorrect type");
	libtw07_dbg_assert(pWriter->m_NumItems < 1024, "too many items");
	libtw07_dbg_assert(Size%sizeof(int) == 0, "incorrect boundary");

	pWriter->m_pItems[pWriter->m_NumItems].m_Type = Type;
	pWriter->m_pItems[pWriter->m_NumItems].m_ID = ID;
	pWriter->m_pItems[pWriter->m_NumItems].m_Size = Size;

	// copy data
	pWriter->m_pItems[pWriter->m_NumItems].m_pData = malloc(Size);
	memcpy(pWriter->m_pItems[pWriter->m_NumItems].m_pData, pData, Size);

	if(!pWriter->m_pItemTypes[Type].m_Num) // count item types
		pWriter->m_NumItemTypes++;

	// link
	pWriter->m_pItems[pWriter->m_NumItems].m_Prev = pWriter->m_pItemTypes[Type].m_Last;
	pWriter->m_pItems[pWriter->m_NumItems].m_Next = -1;

	if(pWriter->m_pItemTypes[Type].m_Last != -1)
		pWriter->m_pItems[pWriter->m_pItemTypes[Type].m_Last].m_Next = pWriter->m_NumItems;
	pWriter->m_pItemTypes[Type].m_Last = pWriter->m_NumItems;

	if(pWriter->m_pItemTypes[Type].m_First == -1)
		pWriter->m_pItemTypes[Type].m_First = pWriter->m_NumItems;

	pWriter->m_pItemTypes[Type].m_Num++;

	pWriter->m_NumItems++;
	return pWriter->m_NumItems-1;
}

int libtw07_datafile_addData(libtw07_datafileWriter *pWriter, int Size, const void *pData)
{
	if(!pWriter->m_File) return 0;

	libtw07_dbg_assert(pWriter->m_NumDatas < 1024, "too much data");

	libtw07_datafileWriter_dataInfo *pInfo = &pWriter->m_pDatas[pWriter->m_NumDatas];
	uLong s = compressBound(Size);
	void *pCompData = malloc(s); // temporary buffer that we use during compression

	int Result = compress((Bytef*)pCompData, &s, (Bytef*)pData, Size);
	if(Result != Z_OK)
	{
		libtw07_print("datafile", "compression error %d", Result);
		libtw07_dbg_assert(0, "zlib error");
	}

	pInfo->m_UncompressedSize = Size;
	pInfo->m_CompressedSize = (int)s;
	pInfo->m_pCompressedData = malloc(pInfo->m_CompressedSize);
	memcpy(pInfo->m_pCompressedData, pCompData, pInfo->m_CompressedSize);
	free(pCompData);

	pWriter->m_NumDatas++;
	return pWriter->m_NumDatas-1;
}

int libtw07_datafile_addDataSwapped(libtw07_datafileWriter *pWriter, int Size, const void *pData)
{
	libtw07_dbg_assert(Size%sizeof(int) == 0, "incorrect boundary");

#if defined(CONF_ARCH_ENDIAN_BIG)
	void *pSwapped = malloc(Size); // temporary buffer that we use during compression
	memcpy(pSwapped, pData, Size);
	swap_endian(pSwapped, sizeof(int), Size/sizeof(int));
	int Index = libtw07_datafile_addData(pWriter, Size, pSwapped);
	free(pSwapped);
	return Index;
#else
	return libtw07_datafile_addData(pWriter, Size, pData);
#endif
}


int libtw07_datafile_writer_finish(libtw07_datafileWriter *pWriter)
{
	if(!pWriter->m_File) return 0;

	int ItemSize = 0;
	int TypesSize, HeaderSize, OffsetSize, FileSize, SwapSize;
	int DataSize = 0;
	libtw07_datafileHeader Header;

	// we should now write this file!
	if(DEBUG)
		libtw07_print("datafile", "writing");

	// calculate sizes
	for(int i = 0; i < pWriter->m_NumItems; i++)
	{
		if(DEBUG)
			libtw07_print("datafile", "item=%d size=%d (%d)", i, pWriter->m_pItems[i].m_Size, (int)(pWriter->m_pItems[i].m_Size+sizeof(libtw07_datafileItem)));
		ItemSize += pWriter->m_pItems[i].m_Size + sizeof(libtw07_datafileItem);
	}


	for(int i = 0; i < pWriter->m_NumDatas; i++)
		DataSize += pWriter->m_pDatas[i].m_CompressedSize;

	// calculate the complete size
	TypesSize = pWriter->m_NumItemTypes*sizeof(libtw07_datafileItemType);
	HeaderSize = sizeof(libtw07_datafileHeader);
	OffsetSize = (pWriter->m_NumItems + pWriter->m_NumDatas + pWriter->m_NumDatas) * sizeof(int); // ItemOffsets, DataOffsets, DataUncompressedSizes
	FileSize = HeaderSize + TypesSize + OffsetSize + ItemSize + DataSize;
	SwapSize = FileSize - DataSize;

	(void)SwapSize;

	if(DEBUG)
		libtw07_print("datafile", "num_m_aItemTypes=%d TypesSize=%d m_aItemsize=%d DataSize=%d", pWriter->m_NumItemTypes, TypesSize, ItemSize, DataSize);

	// construct Header
	{
		Header.m_aID[0] = 'D';
		Header.m_aID[1] = 'A';
		Header.m_aID[2] = 'T';
		Header.m_aID[3] = 'A';
		Header.m_Version = 4;
		Header.m_Size = FileSize - 16;
		Header.m_Swaplen = SwapSize - 16;
		Header.m_NumItemTypes = pWriter->m_NumItemTypes;
		Header.m_NumItems = pWriter->m_NumItems;
		Header.m_NumRawData = pWriter->m_NumDatas;
		Header.m_ItemSize = ItemSize;
		Header.m_DataSize = DataSize;

		// write Header
		if(DEBUG)
			libtw07_print("datafile", "HeaderSize=%d", (int)sizeof(Header));
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Header, sizeof(int), sizeof(Header)/sizeof(int));
#endif
		fwrite(&Header, 1, sizeof(Header), pWriter->m_File);
	}

	// write types
	for(int i = 0, Count = 0; i < 0xffff; i++)
	{
		if(pWriter->m_pItemTypes[i].m_Num)
		{
			// write info
			libtw07_datafileItemType Info;
			Info.m_Type = i;
			Info.m_Start = Count;
			Info.m_Num = pWriter->m_pItemTypes[i].m_Num;
			if(DEBUG)
				libtw07_print("datafile", "writing type=%x start=%d num=%d", Info.m_Type, Info.m_Start, Info.m_Num);
#if defined(CONF_ARCH_ENDIAN_BIG)
			swap_endian(&Info, sizeof(int), sizeof(libtw07_datafileItemType)/sizeof(int));
#endif
			fwrite(&Info, 1, sizeof(Info), pWriter->m_File);
			Count += pWriter->m_pItemTypes[i].m_Num;
		}
	}

	// write item offsets
	for(int i = 0, Offset = 0; i < 0xffff; i++)
	{
		if(pWriter->m_pItemTypes[i].m_Num)
		{
			// write all m_pItems in of this type
			int k = pWriter->m_pItemTypes[i].m_First;
			while(k != -1)
			{
				if(DEBUG)
					libtw07_print("datafile", "writing item offset num=%d offset=%d", k, Offset);
				int Temp = Offset;
#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Temp, sizeof(int), sizeof(Temp)/sizeof(int));
#endif
				fwrite(&Temp, 1, sizeof(Temp), pWriter->m_File);
				Offset += pWriter->m_pItems[k].m_Size + sizeof(libtw07_datafileItem);

				// next
				k = pWriter->m_pItems[k].m_Next;
			}
		}
	}

	// write data offsets
	for(int i = 0, Offset = 0; i < pWriter->m_NumDatas; i++)
	{
		if(DEBUG)
			libtw07_print("datafile", "writing data offset num=%d offset=%d", i, Offset);
		int Temp = Offset;
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&Temp, sizeof(int), sizeof(Temp)/sizeof(int));
#endif
		fwrite(&Temp, 1, sizeof(Temp), pWriter->m_File);
		Offset += pWriter->m_pDatas[i].m_CompressedSize;
	}

	// write data uncompressed sizes
	for(int i = 0; i < pWriter->m_NumDatas; i++)
	{
		if(DEBUG)
			libtw07_print("datafile", "writing data uncompressed size num=%d size=%d", i, pWriter->m_pDatas[i].m_UncompressedSize);
		int UncompressedSize = pWriter->m_pDatas[i].m_UncompressedSize;
#if defined(CONF_ARCH_ENDIAN_BIG)
		swap_endian(&UncompressedSize, sizeof(int), sizeof(UncompressedSize)/sizeof(int));
#endif
		fwrite(&UncompressedSize, 1, sizeof(UncompressedSize), pWriter->m_File);
	}

	// write m_pItems
	for(int i = 0; i < 0xffff; i++)
	{
		if(pWriter->m_pItemTypes[i].m_Num)
		{
			// write all m_pItems in of this type
			int k = pWriter->m_pItemTypes[i].m_First;
			while(k != -1)
			{
				libtw07_datafileItem Item;
				Item.m_TypeAndID = (i<<16)|pWriter->m_pItems[k].m_ID;
				Item.m_Size = pWriter->m_pItems[k].m_Size;
				if(DEBUG)
					libtw07_print("datafile", "writing item type=%x idx=%d id=%d size=%d", i, k, pWriter->m_pItems[k].m_ID, pWriter->m_pItems[k].m_Size);

#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(&Item, sizeof(int), sizeof(Item)/sizeof(int));
				swap_endian(pWriter->m_pItems[k].m_pData, sizeof(int), pWriter->m_pItems[k].m_Size/sizeof(int));
#endif
				fwrite(&Item, 1, sizeof(Item), pWriter->m_File);
				fwrite(pWriter->m_pItems[k].m_pData, 1, pWriter->m_pItems[k].m_Size, pWriter->m_File);

				// next
				k = pWriter->m_pItems[k].m_Next;
			}
		}
	}

	// write data
	for(int i = 0; i < pWriter->m_NumDatas; i++)
	{
		if(DEBUG)
			libtw07_print("datafile", "writing data id=%d size=%d", i, pWriter->m_pDatas[i].m_CompressedSize);
		fwrite(pWriter->m_pDatas[i].m_pCompressedData, 1, pWriter->m_pDatas[i].m_CompressedSize, pWriter->m_File);
	}

	// free data
	for(int i = 0; i < pWriter->m_NumItems; i++)
		free(pWriter->m_pItems[i].m_pData);
	for(int i = 0; i < pWriter->m_NumDatas; ++i)
		free(pWriter->m_pDatas[i].m_pCompressedData);

	fclose(pWriter->m_File);
	pWriter->m_File = NULL;

	if(DEBUG)
		libtw07_print("datafile", "done");
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif // LIBTW07_DATAFILE_H
