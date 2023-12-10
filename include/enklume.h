#ifndef CUNK_ENKLUME_H
#define CUNK_ENKLUME_H

#include <stdint.h>
#include <stdbool.h>

typedef union {
    enum { McVersionUnknown, McVersionRelease, McVersionSnapshot, McVersionOther } tag;
    struct {
        uint8_t major, minor, patch, rc;
    } release;
    struct {
        uint16_t year;
        uint8_t week, patch;
    } snapshot;
} McVersion;

typedef uint32_t McDataVersion;

typedef struct McRegion_ McRegion;
typedef struct McChunk_ McChunk;
typedef struct McWorld_ McWorld;

McWorld* cunk_open_mcworld(const char* folder);
void cunk_close_mcworld(McWorld*);

McRegion* cunk_open_mcregion(McWorld* world, int x, int z);
McChunk* cunk_open_mcchunk(McRegion* world, unsigned int x, unsigned int z);

typedef struct NBT_Object_ NBT_Object;
const NBT_Object* cunk_mcchunk_get_root(const McChunk*);
McDataVersion cunk_mcchunk_get_data_version(const McChunk*);

#endif
