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

#include "external/md5/md5.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint64_t length;
	uint32_t state[8];
	uint32_t curlen;
	unsigned char buf[64];
} SHA256_CTX;
typedef md5_state_t MD5_CTX;

enum
{
	SHA256_DIGEST_LENGTH = 256 / 8,
	SHA256_MAXSTRSIZE = 2 * SHA256_DIGEST_LENGTH + 1,
	MD5_DIGEST_LENGTH = 128 / 8,
	MD5_MAXSTRSIZE = 2 * MD5_DIGEST_LENGTH + 1,
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

// SHA-256. Adapted from https://github.com/kalven/sha-2, which was adapted
// from LibTomCrypt. This code is Public Domain.
typedef uint32_t u32;
typedef uint64_t u64;
typedef SHA256_CTX sha256_state;

static const u32 K[64] =
	{
		0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
		0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
		0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
		0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
		0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
		0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
		0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
		0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
		0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
		0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
		0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
		0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
		0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

static u32 min(u32 x, u32 y)
{
	return x < y ? x : y;
}

static u32 load32(const unsigned char *y)
{
	return ((u32) y[0] << 24) | ((u32) y[1] << 16) | ((u32) y[2] << 8) | ((u32) y[3] << 0);
}

static void store64(u64 x, unsigned char *y)
{
	int i;
	for(i = 0; i != 8; ++i)
		y[i] = (x >> ((7 - i) * 8)) & 255;
}

static void store32(u32 x, unsigned char *y)
{
	int i;
	for(i = 0; i != 4; ++i)
		y[i] = (x >> ((3 - i) * 8)) & 255;
}

static u32 Ch(u32 x, u32 y, u32 z) { return z ^ (x & (y ^ z)); }
static u32 Maj(u32 x, u32 y, u32 z) { return ((x | y) & z) | (x & y); }
static u32 Rot(u32 x, u32 n) { return (x >> (n & 31)) | (x << (32 - (n & 31))); }
static u32 Sh(u32 x, u32 n) { return x >> n; }
static u32 Sigma0(u32 x) { return Rot(x, 2) ^ Rot(x, 13) ^ Rot(x, 22); }
static u32 Sigma1(u32 x) { return Rot(x, 6) ^ Rot(x, 11) ^ Rot(x, 25); }
static u32 Gamma0(u32 x) { return Rot(x, 7) ^ Rot(x, 18) ^ Sh(x, 3); }
static u32 Gamma1(u32 x) { return Rot(x, 17) ^ Rot(x, 19) ^ Sh(x, 10); }

static void sha_compress(sha256_state *md, const unsigned char *buf)
{
	u32 S[8], W[64], t0, t1, t;
	int i;

	// Copy state into S
	for(i = 0; i < 8; i++)
		S[i] = md->state[i];

	// Copy the state into 512-bits into W[0..15]
	for(i = 0; i < 16; i++)
		W[i] = load32(buf + (4 * i));

	// Fill W[16..63]
	for(i = 16; i < 64; i++)
		W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];

// Compress
#define RND(a, b, c, d, e, f, g, h, i) \
	{ \
		t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i]; \
		t1 = Sigma0(a) + Maj(a, b, c); \
		d += t0; \
		h = t0 + t1; \
	}

	for(i = 0; i < 64; ++i)
	{
		RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
		t = S[7];
		S[7] = S[6];
		S[6] = S[5];
		S[5] = S[4];
		S[4] = S[3];
		S[3] = S[2];
		S[2] = S[1];
		S[1] = S[0];
		S[0] = t;
	}

	// Feedback
	for(i = 0; i < 8; i++)
		md->state[i] = md->state[i] + S[i];
}

// Public interface

static void sha_init(sha256_state *md)
{
	md->curlen = 0;
	md->length = 0;
	md->state[0] = 0x6A09E667UL;
	md->state[1] = 0xBB67AE85UL;
	md->state[2] = 0x3C6EF372UL;
	md->state[3] = 0xA54FF53AUL;
	md->state[4] = 0x510E527FUL;
	md->state[5] = 0x9B05688CUL;
	md->state[6] = 0x1F83D9ABUL;
	md->state[7] = 0x5BE0CD19UL;
}

static void sha_process(sha256_state *md, const void *src, u32 inlen)
{
	const u32 block_size = 64;
	const unsigned char *in = (const unsigned char *) src;

	while(inlen > 0)
	{
		if(md->curlen == 0 && inlen >= block_size)
		{
			sha_compress(md, in);
			md->length += block_size * 8;
			in += block_size;
			inlen -= block_size;
		}
		else
		{
			u32 n = min(inlen, (block_size - md->curlen));
			memcpy(md->buf + md->curlen, in, n);
			md->curlen += n;
			in += n;
			inlen -= n;

			if(md->curlen == block_size)
			{
				sha_compress(md, md->buf);
				md->length += 8 * block_size;
				md->curlen = 0;
			}
		}
	}
}

static void sha_done(sha256_state *md, void *out)
{
	int i;

	// Increase the length of the message
	md->length += md->curlen * 8;

	// Append the '1' bit
	md->buf[md->curlen++] = (unsigned char) 0x80;

	// If the length is currently above 56 bytes we append zeros then compress.
	// Then we can fall back to padding zeros and length encoding like normal.
	if(md->curlen > 56)
	{
		while(md->curlen < 64)
			md->buf[md->curlen++] = 0;
		sha_compress(md, md->buf);
		md->curlen = 0;
	}

	// Pad up to 56 bytes of zeroes
	while(md->curlen < 56)
		md->buf[md->curlen++] = 0;

	// Store length
	store64(md->length, md->buf + 56);
	sha_compress(md, md->buf);

	// Copy output
	for(i = 0; i < 8; i++)
		store32(md->state[i], (unsigned char *) out + (4 * i));
}

void sha256_init(SHA256_CTX *ctxt)
{
	sha_init(ctxt);
}

void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len)
{
	sha_process(ctxt, data, data_len);
}

SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt)
{
	SHA256_DIGEST result;
	sha_done(ctxt, result.data);
	return result;
}

void md5_update(MD5_CTX *ctxt, const void *data, size_t data_len)
{
	md5_append(ctxt, (md5_byte_t *) data, data_len);
}

MD5_DIGEST md5_finish(MD5_CTX *ctxt)
{
	MD5_DIGEST result;
	md5_finish_(ctxt, result.data);
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
