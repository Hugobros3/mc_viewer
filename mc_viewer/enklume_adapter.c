#include "enklume.h"
#include "nbt.h"

#include "chunk.h"

#include "cunk/util.h"

#include <string.h>
#include <assert.h>

#define MC_1_18_DATA_VERSION 2825

static void decode_pre_flattening(Chunk* dst_chunk, int section_y, const NBT_Object* blocks_data) {
    if (!blocks_data)
        return;
    const NBT_ByteArray* arr = cunk_nbt_extract_byte_array(blocks_data);
    for (int pos = 0; pos < 16 * 16 * 16; pos++) {
        int x = (pos >> 4) & 15;
        int y = (pos >> 8) & 15;
        int z = (pos >> 0) & 15;

        int8_t block_state = arr->arr[pos];
        assert(pos < arr->count);

        chunk_set_block_data(dst_chunk, x, y + section_y * 16, z, (block_state == 0) ? 0 : 1);
    }
}

static void decode_post_flattening(Chunk* dst_chunk, int section_y, const NBT_Object* block_states, const NBT_Object* palette, bool can_straddle_boundary) {
    if (!(block_states && palette))
        return;
    // cunk_print_nbt(p, palette);
    assert(block_states->tag == NBT_Tag_LongArray && palette->tag == NBT_Tag_List);
    const NBT_LongArray* block_state_arr = cunk_nbt_extract_long_array(block_states);
    int palette_size = palette->body.p_list.count;

    bool is_air[palette_size];
    for (size_t j = 0; j < palette_size; j++) {
        const NBT_Compound* color = &palette->body.p_list.bodies[j].p_compound;
        const char* name = *cunk_nbt_extract_string(cunk_nbt_compound_direct_access(color, "Name"));
        assert(name);
        if (strcmp(name, "minecraft:air") == 0)
            is_air[j] = true;
    }

    int bits = needed_bits(palette_size);
    if (bits < 4)
        bits = 4;

    int aligned_bitpos = 0;
    for (int pos = 0; pos < 16 * 16 * 16; pos++) {
        int x = (pos >> 4) & 15;
        int y = (pos >> 8) & 15;
        int z = (pos >> 0) & 15;

        uint64_t block_state = fetch_bits_long_arr(block_state_arr->arr, true, aligned_bitpos, bits);
        aligned_bitpos += bits;
        // "Since 1.16, the indices are not packed across multiple elements of the array, meaning that if there is no more space in a given 64-bit integer for the next index, it starts instead at the first (lowest) bit of the next 64-bit element."
        // https://minecraft.fandom.com/wiki/Chunk_format#NBT_structure
        if (!can_straddle_boundary) {
            int starting_long = aligned_bitpos / 64;
            int finishing_long = (aligned_bitpos + bits - 1) / 64;
            if (starting_long != finishing_long)
                aligned_bitpos = finishing_long * 64;
        }

        assert(block_state < palette_size);
        block_state %= palette_size;
        chunk_set_block_data(dst_chunk, x, y + section_y * 16, z, is_air[block_state] ? 0 : 1);
    }
}

void load_from_mcchunk(Chunk* dst_chunk, McChunk* chunk) {
    McDataVersion ver = cunk_mcchunk_get_data_version(chunk);
    bool post_1_18 = ver > MC_1_18_DATA_VERSION;

    const NBT_Object *o = cunk_mcchunk_get_root(chunk);
    assert(o);

    const NBT_Object *level = cunk_nbt_compound_access(o, "Level");
    if (level)
        o = level;
    else
        assert(post_1_18);
    assert(o);
    // Iterate over sections
    o = cunk_nbt_compound_access(o, post_1_18 ? "sections" : "Sections");
    assert(o);
    const NBT_List* sections = cunk_nbt_extract_list(o);
    assert(sections);
    assert(sections->tag == NBT_Tag_Compound);
    for (size_t i = 0; i < sections->count; i++) {
        const NBT_Compound* section = &sections->bodies[i].p_compound;
        int8_t section_y = *cunk_nbt_extract_byte(cunk_nbt_compound_direct_access(section, "Y"));
        if (section_y < 0)
            continue;

        const NBT_Object* blocks_data = cunk_nbt_compound_direct_access(section, "Blocks");
        if (blocks_data) {
            decode_pre_flattening(dst_chunk, section_y, blocks_data);
        } else {
            // Starting with 1.18, block data for a chunk section is stored in a 'block_states' compound
            const NBT_Compound* container = section;
            const NBT_Object* block_states_container = cunk_nbt_compound_direct_access(section, "block_states");
            if (post_1_18 && block_states_container)
                container = cunk_nbt_extract_compound(block_states_container);

            const NBT_Object* block_states = cunk_nbt_compound_direct_access(container, post_1_18 ? "data" : "BlockStates");
            const NBT_Object* palette = cunk_nbt_compound_direct_access(container, post_1_18 ? "palette" : "Palette");
            decode_post_flattening(dst_chunk, section_y, block_states, palette, ver < 2504);
        }
    }
}
