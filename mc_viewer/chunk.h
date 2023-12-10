#ifndef CUNK_CHUNK
#define CUNK_CHUNK

#include <stdint.h>
#include <stddef.h>

#define CUNK_CHUNK_SIZE 16
#define CUNK_CHUNK_MAX_HEIGHT 384
#define CUNK_CHUNK_SECTIONS_COUNT CUNK_CHUNK_MAX_HEIGHT / CUNK_CHUNK_SIZE

typedef uint32_t BlockData;

static BlockData air_data = 0;

typedef struct {
    BlockData block_data[CUNK_CHUNK_SIZE][CUNK_CHUNK_SIZE][CUNK_CHUNK_SIZE];
} ChunkSection;

typedef struct {
    int x, y, z;
    ChunkSection* sections[CUNK_CHUNK_SECTIONS_COUNT];
} Chunk;

BlockData chunk_get_block_data(const Chunk*, unsigned x, unsigned y, unsigned z);
void chunk_set_block_data(Chunk*, unsigned x, unsigned y, unsigned z, BlockData);

typedef struct Growy_ Growy;
void chunk_mesh(const Chunk* chunk, const Chunk* neighbours[3][3][3], Growy* g, size_t* num_verts);

#endif
