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
#ifndef LIBTW07_HASH_H
#define LIBTW07_HASH_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define OPENSSL_API_COMPAT 0x00908000L
#include <openssl/md5.h>
#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	SHA256_MAXSTRSIZE=2*SHA256_DIGEST_LENGTH+1,
	MD5_MAXSTRSIZE=2*MD5_DIGEST_LENGTH+1,
};

typedef struct
{
	unsigned char data[SHA256_DIGEST_LENGTH];
} SHA256_DIGEST;

typedef struct
{
	unsigned char data[MD5_DIGEST_LENGTH];
} MD5_DIGEST;

SHA256_DIGEST sha256(const void *message, size_t message_len);
void sha256_str(SHA256_DIGEST digest, char *str, size_t max_len);
int sha256_comp(SHA256_DIGEST digest1, SHA256_DIGEST digest2);

MD5_DIGEST md5(const void *message, size_t message_len);
void md5_str(MD5_DIGEST digest, char *str, size_t max_len);
int md5_comp(MD5_DIGEST digest1, MD5_DIGEST digest2);

static const SHA256_DIGEST SHA256_ZEROED = {{0}};
static const MD5_DIGEST MD5_ZEROED = {{0}};

void sha256_init(SHA256_CTX *ctxt);
void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len);
SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt);

void md5_init(MD5_CTX *ctxt);
void md5_update(MD5_CTX *ctxt, const void *data, size_t data_len);
MD5_DIGEST md5_finish(MD5_CTX *ctxt);

static void digest_str(const unsigned char *digest, size_t digest_len, char *str, size_t max_len)
{
	unsigned i;
	if(max_len > digest_len * 2 + 1)
	{
		max_len = digest_len * 2 + 1;
	}
	str[max_len - 1] = 0;
	max_len -= 1;
	for(i = 0; i < max_len; i++)
	{
		static const char HEX[] = "0123456789abcdef";
		int index = i / 2;
		if(i % 2 == 0)
		{
			str[i] = HEX[digest[index] >> 4];
		}
		else
		{
			str[i] = HEX[digest[index] & 0xf];
		}
	}
}

SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt)
{
	SHA256_DIGEST result;
	SHA256_Final(result.data, ctxt);
	return result;
}

SHA256_DIGEST sha256(const void *message, size_t message_len)
{
	SHA256_CTX ctxt;
	sha256_init(&ctxt);
	sha256_update(&ctxt, message, message_len);
	return sha256_finish(&ctxt);
}

void sha256_str(SHA256_DIGEST digest, char *str, size_t max_len)
{
	digest_str(digest.data, sizeof(digest.data), str, max_len);
}

int sha256_comp(SHA256_DIGEST digest1, SHA256_DIGEST digest2)
{
	return memcmp(digest1.data, digest2.data, sizeof(digest1.data));
}

MD5_DIGEST md5(const void *message, size_t message_len)
{
	MD5_CTX ctxt;
	md5_init(&ctxt);
	md5_update(&ctxt, message, message_len);
	return md5_finish(&ctxt);
}

void md5_str(MD5_DIGEST digest, char *str, size_t max_len)
{
	digest_str(digest.data, sizeof(digest.data), str, max_len);
}

int md5_comp(MD5_DIGEST digest1, MD5_DIGEST digest2)
{
	return memcmp(digest1.data, digest2.data, sizeof(digest1.data));
}

void sha256_init(SHA256_CTX *ctxt)
{
	SHA256_Init(ctxt);
}

void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len)
{
	SHA256_Update(ctxt, data, data_len);
}

void md5_init(MD5_CTX *ctxt)
{
	MD5_Init(ctxt);
}

void md5_update(MD5_CTX *ctxt, const void *data, size_t data_len)
{
	MD5_Update(ctxt, data, data_len);
}

MD5_DIGEST md5_finish(MD5_CTX *ctxt)
{
	MD5_DIGEST result;
	MD5_Final(result.data, ctxt);
	return result;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
inline bool operator==(const SHA256_DIGEST &that, const SHA256_DIGEST &other)
{
	return sha256_comp(that, other) == 0;
}
inline bool operator!=(const SHA256_DIGEST &that, const SHA256_DIGEST &other)
{
	return !(that == other);
}
inline bool operator==(const MD5_DIGEST &that, const MD5_DIGEST &other)
{
	return md5_comp(that, other) == 0;
}
inline bool operator!=(const MD5_DIGEST &that, const MD5_DIGEST &other)
{
	return !(that == other);
}
#endif

#endif // BASE_HASH_H
