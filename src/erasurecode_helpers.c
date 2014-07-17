/*
 * <Copyright>
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
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * liberasurecode API helpers implementation
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#include "erasurecode_helpers.h"

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

void *get_aligned_buffer16(int size)
{
    void *buf;

    /**
     * Ensure all memory is aligned to 16-byte boundaries
     * to support 128-bit operations
     */
    if (posix_memalign(&buf, 16, size) < 0) {
        return NULL;
    }

    bzero(buf, size);

    return buf;
}

char *alloc_fragment_buffer(int size)
{
    char *buf;
    fragment_header_t *header = NULL;

    size += sizeof(fragment_header_t);
    get_aligned_buffer16(size);

    header = (fragment_header_t *) buf;
    header->magic = PYECC_HEADER_MAGIC;

    return buf;
}

int free_fragment_buffer(char *buf)
{
    fragment_header_t *header;

    if (buf == NULL) {
        return -1;
    }

    buf -= sizeof(fragment_header_t);

    header = (fragment_header_t *) buf;
    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr,
            "Invalid fragment header (free fragment)!");
        return -1;
    }

    free(buf);
    return 0;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

char *get_data_ptr_from_fragment(char *buf)
{
    buf += sizeof(fragment_header_t);

    return buf;
}

char *get_fragment_ptr_from_data_novalidate(char *buf)
{
    buf -= sizeof(fragment_header_t);

    return buf;
}

char *get_fragment_ptr_from_data(char *buf)
{
    fragment_header_t *header;

    buf -= sizeof(fragment_header_t);

    header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (get header ptr)!\n");
        return NULL;
    }

    return buf;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

int set_fragment_idx(char *buf, int idx)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (idx check)!\n");
        return -1;
    }

    header->idx = idx;

    return 0;
}

int get_fragment_idx(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (get idx)!");
        return -1;
    }

    return header->idx;
}

int set_fragment_size(char *buf, int size)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (size check)!");
        return -1;
    }

    header->size = size;

    return 0;
}

int get_fragment_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (get size)!");
        return -1;
    }

    return header->size;
}

int set_orig_data_size(char *buf, int orig_data_size)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (set orig data check)!");
        return -1;
    }

    header->orig_data_size = orig_data_size;

    return 0;
}

int get_orig_data_size(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        fprintf(stderr, "Invalid fragment header (get orig data check)!");
        return -1;
    }

    return header->orig_data_size;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

int validate_fragment(char *buf)
{
    fragment_header_t *header = (fragment_header_t *) buf;

    if (header->magic != PYECC_HEADER_MAGIC) {
        return -1;
    }

    return 0;
}

/* ==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~==~=*=~== */

