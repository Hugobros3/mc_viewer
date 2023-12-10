#include "chunk.h"

#include <stdlib.h>
#include <assert.h>

BlockData chunk_get_block_data(const Chunk* chunk, unsigned x, unsigned y, unsigned z) {
    assert(x < CUNK_CHUNK_SIZE && z < CUNK_CHUNK_SIZE && y < CUNK_CHUNK_MAX_HEIGHT);
    const ChunkSection* section = chunk->sections[y / CUNK_CHUNK_SIZE];
    y %= CUNK_CHUNK_SIZE;
    if (section)
        return section->block_data[y][z][x];
    return air_data;
}

void chunk_set_block_data(Chunk* chunk, unsigned x, unsigned y, unsigned z, BlockData data) {
    assert(x < CUNK_CHUNK_SIZE && z < CUNK_CHUNK_SIZE && y < CUNK_CHUNK_MAX_HEIGHT);
    unsigned sid = y / CUNK_CHUNK_SIZE;
    ChunkSection* section = chunk->sections[sid];
    y %= CUNK_CHUNK_SIZE;
    if (!section)
        section = chunk->sections[sid] = calloc(CUNK_CHUNK_SIZE * CUNK_CHUNK_SIZE * CUNK_CHUNK_SIZE, sizeof(BlockData));
    section->block_data[y][z][x] = data;
}
