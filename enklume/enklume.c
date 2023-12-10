#include "enklume.h"
#include "nbt.h"
#include "zlib_wrap.h"

#include "cunk/memory.h"
#include "cunk/print.h"
#include "cunk/io.h"
#include "cunk/util.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdalign.h>

struct McWorld_ {
    Arena* arena;
    const char* path;
};

McWorld* cunk_open_mcworld(const char* folder) {
    if (!folder_exists(folder))
        return NULL;
    const char* datfile_path = format_string("%s/level.dat", folder);
    if (!file_exists(datfile_path))
        return NULL;
    free((char*) datfile_path);

    Arena* arena = cunk_new_arena();

    McWorld* world = calloc(1, sizeof(McWorld));
    *world = (McWorld) {
            .arena = arena,
            .path = cunk_arena_string(arena, folder),
    };
    return world;
}

void cunk_close_mcworld(McWorld* w_) {
    McWorld* w = (McWorld*) w_;
    cunk_arena_destroy(w->arena);
    free(w);
}

typedef union {
    struct {
        unsigned offset: 24;
        unsigned sector_count: 8;
    };
    uint32_t word;
} ChunkLocation;

typedef uint32_t ChunkTimestamp;

typedef struct {
    ChunkLocation locations[32][32];
    ChunkTimestamp timestamps[32][32];
} McRegionHeader;

static_assert(sizeof(ChunkLocation) == sizeof(int32_t), "some unwanted padding made it in :/");

typedef enum {
    Compr_INVALID, Compr_GZip, Compr_Zlib, Compr_Uncompressed
} McChunkCompression;

typedef struct {
    uint32_t length;
    uint8_t compression_type;
    const char* compressed_data;
} McRegionPayload;

struct McRegion_ {
    char* bytes;
    McRegionHeader decoded_header;
    McRegionPayload decoded_payloads[32][32];
};

McRegion* cunk_open_mcregion(McWorld* world, int x, int z) {
    const char* path = format_string("%s/region/r.%d.%d.mca", world->path, x, z);
    if (!file_exists(path))
        goto fail;

    size_t size;
    char* contents;
    if (!read_file(path, &size, &contents))
        goto fail;

    McRegion* region = calloc(1, sizeof(McRegion));

    // decode the headers
    const McRegionHeader* big_endian_header = contents;
    for (int cz = 0; cz < 32; cz++) {
        for (int cx = 0; cx < 32; cx++) {
            // We need to swap the endianness of those
            ChunkLocation location = region->decoded_header.locations[cz][cx];
            location.offset = swap_endianness(3, big_endian_header->locations[cz][cx].offset & 0xFFFFFF);
            location.sector_count = swap_endianness(1, big_endian_header->locations[cz][cx].sector_count & 0xFF);
            region->decoded_header.locations[cz][cx] = location;
            // Likewise.
            region->decoded_header.timestamps[cz][cx] = swap_endianness(4, big_endian_header->timestamps[cz][cx]);

            assert(region->decoded_header.locations[cz][cx].offset * 4096 < size);
            McRegionPayload* payload = &region->decoded_payloads[cz][cx];

            if (location.sector_count > 0) {
                const McRegionPayload* big_endian_payload = contents + location.offset * 4096;
                payload->length = swap_endianness(4, big_endian_payload->length);
                // payload->compression_type = cunk_swap_endianness(4, big_endian_payload->compression_type);
                payload->compression_type = big_endian_payload->compression_type;
                assert((McChunkCompression) payload->compression_type <= Compr_Uncompressed);
                payload->compressed_data = (char *) big_endian_payload + 5;

                assert((size_t) (location.offset * 4096 + payload->length) <= size);
            } else {
                payload->length = 0;
                payload->compressed_data = NULL;
            }
        }
    }

    region->bytes = contents;
    return region;

    fail:
    free((char*) path);
    return NULL;
}

struct McChunk_ {
    Arena* a;
    NBT_Object* root;
};

McChunk* cunk_open_mcchunk(McRegion* region, unsigned int x, unsigned int z) {
    assert(x < 32 && z < 32);

    Arena* a = cunk_new_arena();
    NBT_Object* root = NULL;
    McRegionPayload* payload = &region->decoded_payloads[z][x];
    if (payload->length == 0)
        return NULL;
    const char* nbt_data = payload->compressed_data;
    uint32_t nbt_data_size = payload->length;
    switch ((McChunkCompression) payload->compression_type) {
        case Compr_Zlib:
        case Compr_GZip: {
            Growy* g = cunk_new_growy();
            ZLibMode zlib_mode = payload->compression_type == Compr_GZip ? ZLib_GZip : ZLib_Zlib;
            assert(payload->compression_type == 2);
            assert(zlib_mode == ZLib_Zlib);
            cunk_inflate(zlib_mode, (size_t) nbt_data_size, nbt_data, g);
            nbt_data_size = cunk_growy_size(g);
            nbt_data = cunk_growy_deconstruct(g);
            root = cunk_decode_nbt(nbt_data_size, nbt_data, a);
            free((char*) nbt_data);
            break;
        }
        case Compr_Uncompressed: {
            root = cunk_decode_nbt(nbt_data_size, nbt_data, a);
            break;
        }
    }

    assert(root);

    McChunk* chunk = calloc(1, sizeof(McChunk));
    *chunk = (McChunk) {
        .a = a,
        .root = root,
    };
    return chunk;
}

const NBT_Object* cunk_mcchunk_get_root(const McChunk* c) { return c->root; }

McDataVersion cunk_mcchunk_get_data_version(const McChunk* c) {
    const NBT_Object* o = cunk_nbt_compound_access(c->root, "DataVersion");
    if (o)
        return *cunk_nbt_extract_int(o);
    return 0;
}
