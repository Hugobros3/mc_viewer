#include "zlib_wrap.h"

#include "cunk/memory.h"

#include <zlib.h>
#include <stdio.h>

#define ZLIB_CHUNK_SIZE 4096

/* report a zlib or i/o error */
void zerr(int ret) {
    fputs("zpipe: ", stderr);
    switch (ret) {
        case Z_ERRNO:
            if (ferror(stdin))
                fputs("error reading stdin\n", stderr);
            if (ferror(stdout))
                fputs("error writing stdout\n", stderr);
            break;
        case Z_STREAM_ERROR:
            fputs("invalid compression level\n", stderr);
            break;
        case Z_DATA_ERROR:
            fputs("invalid or incomplete deflate data\n", stderr);
            break;
        case Z_MEM_ERROR:
            fputs("out of memory\n", stderr);
            break;
        case Z_VERSION_ERROR:
            fputs("zlib version mismatch!\n", stderr);
    }
}

#pragma GCC diagnostic error "-Wswitch"

static int format_bits(ZLibMode mode) {
    switch (mode) {
        case ZLib_Deflate: return -MAX_WBITS;
        case ZLib_Zlib:    return MAX_WBITS;
        case ZLib_GZip:    return MAX_WBITS | 16;
    }
}

bool cunk_inflate(ZLibMode mode, size_t src_size, const char* input_data, Growy* output) {
    int ret;
    z_stream strm;
    unsigned char out[ZLIB_CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, format_bits(mode));
    if (ret != Z_OK)
        return false;

    strm.avail_in = src_size;
    strm.next_in = (unsigned char*) input_data;
    do {
        strm.avail_out = ZLIB_CHUNK_SIZE;
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        size_t got = ZLIB_CHUNK_SIZE - strm.avail_out;
        cunk_growy_append_bytes(output, got, (char*) out);
        if (ret == Z_STREAM_END)
            break;
        if (ret)
            zerr(ret);
    } while (true);

    inflateEnd(&strm);
    return true;
}
