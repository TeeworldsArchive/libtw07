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
#ifndef LIBTW07_PRINT_H
#define LIBTW07_PRINT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

static int libtw07_enable_print = 0;

// modify from dbg_msg
int libtw07_print(const char *from, const char *fmt, ...)
{
	if(libtw07_enable_print == 0)
		return -1;

	printf("[%s]: ", from);

	char str[1024*4];
	va_list args;
	va_start(args, fmt);
#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	_vsprintf_p(str, sizeof(str), fmt, args);
#else
	vsnprintf(str, sizeof(str), fmt, args);
#endif
	va_end(args);
    return printf("%s\n", str);
}

void libtw07_dbg_assert_imp(const char *filename, int line, int test, const char *msg)
{
	if(!test)
	{
		libtw07_print("assert", "%s(%d): %s", filename, line, msg);
		abort();
	}
}
#define libtw07_dbg_assert(test,msg) libtw07_dbg_assert_imp(__FILE__, __LINE__, test, msg)


#ifdef __cplusplus
}
#endif

#endif // LIBTW07_PRINT_H
