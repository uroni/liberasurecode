/*
 * Copyright(c) 2015 NTT corp. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
 * THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;LOSS OF USE,
 * DATA, OR PROFITS;OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * liberasurecode shss backend
 *
 * Please contact us if you are insterested in the NTT backend (welcome!):
 * Kota Tsuyuzaki <tsuyuzaki.kota@lab.ntt.co.jp>
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>

#include "erasurecode.h"
#include "erasurecode_helpers.h"
#include "erasurecode_backend.h"

/* Forward declarations */
struct ec_backend shss;
struct ec_backend_op_stubs shss_ops;

typedef int (*shss_encode_func)(char **, size_t, int, int, int, int, long long *);
typedef int (*shss_decode_func)(char **, size_t, int *, int, int, int, int, int, long long *);
typedef int (*shss_reconstruct_func)(char **, size_t, int *, int, int *, int, int, int, int, int, long long *);

struct shss_descriptor {
    /* calls required for init */
    shss_encode_func ssencode;
    shss_decode_func ssdecode;
    shss_reconstruct_func ssreconst;

    /* fields needed to hold state */
    int k;
    int m;
    int n;
    int w;
    int aes_bit_length;
};

#define SHSS_LIB_VER_STR "1.0"
#define SHSS_LIB_NAME "shss"
#if defined(__MACOS__) || defined(__MACOSX__) || defined(__OSX__) || defined(__APPLE__)
#define SHSS_SO_NAME "libshss.dylib"
#else
#define SHSS_SO_NAME "libshss.so"
#endif
#define DEFAULT_W 128

/* TODO:
   metadata_adder is still in discussion. shss needs to a fixed value to allocate extra bytes
   for *each* fragment. However, current liberasurecode calculates the extra bytes as
   "(alined_data_size + metadata_adder) / k" so that shss has to define the METADATA as a big value
   to alloc enough memory for the maximum number of k even if k is smaller than the maximum value.

   i.e. (shss specification is)
   Enough Extra Bytes (for *each* fragment): 32
   The Maximum Number: 20 (k=20)
*/
#define METADATA 32 * 20

static int shss_encode(void *desc, char **data, char **parity,
        int blocksize)
{
    int i;
    int ret = 0;
    int priv_bitnum = 128; // privacy bit number 0 or 128(default) or 256
    int chksum = 0; // chksum 0 or 64
    char **encoded;
    long long einfo;
    struct shss_descriptor *xdesc =
        (struct shss_descriptor *) desc;

    if (xdesc->aes_bit_length != -1) {
        priv_bitnum = xdesc->aes_bit_length;
    }

    encoded = alloca(sizeof(char*)*xdesc->n);

    for (i = 0; i<xdesc->k; i++) encoded[i] = (char*)data[i];
    for (i = 0; i<xdesc->m; i++) encoded[i+xdesc->k] = (char*)parity[i];

    ret = xdesc->ssencode((char**)encoded, (size_t)blocksize,
                        xdesc->k, xdesc->m, priv_bitnum, chksum, &einfo);

    if (ret > 0) {
        return -ret;
    }

    return 0;
}

static int shss_decode(void *desc, char **data, char **parity,
        int *missing_idxs, int blocksize)
{
    int i;
    int missing_size = 0;
    int ret = 0;
    int priv_bitnum = 128; // privacy bit number 0 or 128(default) or 256
    int chksum = 0; // chksum 0 or 64
    char **decoded;
    long long einfo;
    struct shss_descriptor *xdesc =
        (struct shss_descriptor *) desc;

    if (xdesc->aes_bit_length != -1) {
        priv_bitnum = xdesc->aes_bit_length;
    }

    decoded = alloca(sizeof(char*)*xdesc->n);

    for (i = 0; i<xdesc->k; i++) decoded[i] = (char*)data[i];
    for (i = 0; i<xdesc->m; i++) decoded[i+xdesc->k] = (char*)parity[i];
    for (i = 0; i<xdesc->n; i++) {
        if (i == missing_idxs[missing_size]) {
            missing_size++;
        }
    }

    ret = xdesc->ssdecode((char**)decoded, (size_t)blocksize, missing_idxs, missing_size,
                         xdesc->k, xdesc->m, priv_bitnum, chksum, &einfo);

    if (ret > 0) {
        return -ret;
    }

    return 0;
}

static int shss_reconstruct(void *desc, char **data, char **parity,
        int *missing_idxs, int destination_idx, int blocksize)
{
    int i;
    int missing_size = 0;
    int ret = 0;
    int priv_bitnum = 128; // privacy bit number 0 or 128(default) or 256
    int chksum = 0; // chksum 0 or 64
    int dst_size = 1;
    char **reconstructed;
    long long einfo;
    struct shss_descriptor *xdesc =
        (struct shss_descriptor *) desc;

    if (xdesc->aes_bit_length != -1) {
        priv_bitnum = xdesc->aes_bit_length;
    }

    reconstructed = alloca(sizeof(char*)*xdesc->n);

    for (i = 0; i<xdesc->k; i++) reconstructed[i] = (char*)data[i];
    for (i = 0; i<xdesc->m; i++) reconstructed[i+xdesc->k] = (char*)parity[i];
    for (i = 0; i<xdesc->n; i++) {
        if (i == missing_idxs[missing_size]) {
            missing_size++;
        }
    }

    ret = xdesc->ssreconst((char**)reconstructed, (size_t)blocksize,
                          &destination_idx, dst_size, missing_idxs, missing_size, xdesc->k,
                          xdesc->m, priv_bitnum, chksum, &einfo);

    if (ret > 0) {
        return -ret;
    }

    return 0;
}

static int shss_fragments_needed(void *desc, int *missing_idxs,
        int *fragments_to_exclude, int *fragments_needed)
{
    struct shss_descriptor *xdesc =
        (struct shss_descriptor *) desc;
    uint64_t exclude_bm = convert_list_to_bitmap(fragments_to_exclude);
    uint64_t missing_bm = convert_list_to_bitmap(missing_idxs) | exclude_bm;
    int i;
    int j = 0;
    int ret = -101;

    for (i = 0; i < xdesc->n; i++) {
        if (!(missing_bm & (1 << i))) {
            fragments_needed[j] = i;
            j++;
        }
        if (j == xdesc->k) {
            ret = 0;
            fragments_needed[j] = -1;
            break;
        }
    }

    return ret;
}

/**
 * Return the element-size, which is the number of bits stored
 * on a given device, per codeword.  This is usually just 'w'.
 */
static int shss_element_size(void* desc)
{
    return DEFAULT_W;
}

static void * shss_init(struct ec_backend_args *args, void *backend_sohandle)
{
    static struct shss_descriptor xdesc;

    xdesc.k = args->uargs.k;
    xdesc.m = args->uargs.m;
    xdesc.n = args->uargs.k + args->uargs.m;
    xdesc.w = DEFAULT_W;
    args->uargs.w = DEFAULT_W;

    /* Sample on how to pass extra args to the backend */
    // TODO: Need discussion how to pass extra args.
    int *priv = (int *)args->uargs.priv_args2;
    xdesc.aes_bit_length = priv[0]; // AES bit number

    union {
        shss_encode_func encodep;
        shss_decode_func decodep;
        shss_reconstruct_func reconp;
        void *vptr;
    } func_handle;

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "ssencode");
    xdesc.ssencode = func_handle.encodep;
    if (NULL == xdesc.ssencode) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "ssdecode");
    xdesc.ssdecode = func_handle.decodep;
    if (NULL == xdesc.ssdecode) {
        goto error;
    }

    func_handle.vptr = NULL;
    func_handle.vptr = dlsym(backend_sohandle, "ssreconst");
    xdesc.ssreconst = func_handle.reconp;
    if (NULL == xdesc.ssreconst) {
        goto error;
    }

    return (void *)&xdesc;

error:
    return NULL;
}

static int shss_exit(void *desc)
{
    return 0;
}

struct ec_backend_op_stubs shss_op_stubs = {
    .INIT                       = shss_init,
    .EXIT                       = shss_exit,
    .ENCODE                     = shss_encode,
    .DECODE                     = shss_decode,
    .FRAGSNEEDED                = shss_fragments_needed,
    .RECONSTRUCT                = shss_reconstruct,
    .ELEMENTSIZE                = shss_element_size,
};

struct ec_backend_common backend_shss = {
    .id                         = EC_BACKEND_SHSS,
    .name                       = SHSS_LIB_NAME,
    .soname                     = SHSS_SO_NAME,
    .soversion                  = SHSS_LIB_VER_STR,
    .ops                        = &shss_op_stubs,
    .metadata_adder             = METADATA,
};