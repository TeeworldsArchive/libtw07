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
#include "../lib/print.h"
#include "../lib/datafile.h"

#include "test.h"

int main(int argc, const char **argv)
{
    libtw07_enable_print = 1;

    libtw07_datafileReader Reader;
    libtw07_datafile_reader_init(&Reader);
    libtw07_datafile_reader_open(&Reader, "test.file");
    if(libtw07_datafile_reader_isOpen(&Reader) == -1)
        return -1;

    uint32_t Crc = libtw07_datafile_reader_crc(&Reader);
    SHA256_DIGEST Sha256 = libtw07_datafile_reader_sha256(&Reader);

    char aSha256[SHA256_MAXSTRSIZE];
    sha256_str(Sha256, aSha256, sizeof(aSha256));

    libtw07_print("test", "crc is %u, sha256 is %s", Crc, aSha256);

	int Start, Num;
	libtw07_datafile_reader_getType(&Reader, 0, &Start, &Num);
	for(int i = 0; i < Num; i++)
	{
		libtw07_testFile *pFile = (libtw07_testFile *) libtw07_datafile_reader_getItem(&Reader, Start + i, 0, 0);
        libtw07_print("test", "file%d flag is %d, string is %s", i, pFile->Flag, pFile->String);
	}

    libtw07_datafile_reader_close(&Reader);
    libtw07_datafile_reader_destroy(&Reader);
    return 0;
}
