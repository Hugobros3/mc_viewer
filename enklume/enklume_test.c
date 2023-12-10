#include "nbt.h"
#include "enklume.h"

#include "cunk/print.h"
#include "cunk/util.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

int main(int argc, char** argv) {
    Printer* p = cunk_open_file_as_printer(stdout);

    McWorld* w = cunk_open_mcworld(argv[1]);
    assert(w);

    McRegion* r = cunk_open_mcregion(w, 0, 0);
    assert(r);
    McChunk* c = cunk_open_mcchunk(r, 0, 0);
    assert(c);

    // cunk_print_nbt(p, cunk_mcchunk_get_root(c));
    const NBT_Object* o = cunk_mcchunk_get_root(c);
    assert(o);
    o = cunk_nbt_compound_access(o, "Level");
    assert(o);
    o = cunk_nbt_compound_access(o, "Sections");
    assert(o);
    const NBT_List* sections = cunk_nbt_extract_list(o);
    assert(sections);
    assert(sections->tag == NBT_Tag_Compound);
    for (size_t i = 0; i < sections->count; i++) {
        const NBT_Compound* section = &sections->bodies[i].p_compound;
        int8_t y = *cunk_nbt_extract_byte(cunk_nbt_compound_direct_access(section, "Y"));
        printf("Y: %d\n", y);
        const NBT_Object* block_states = cunk_nbt_compound_direct_access(section, "BlockStates");
        const NBT_Object* palette = cunk_nbt_compound_direct_access(section, "Palette");
        if (!(block_states && palette))
            continue;
        cunk_print_nbt(p, palette);
        assert(block_states->tag == NBT_Tag_LongArray && palette->tag == NBT_Tag_List);
        const NBT_LongArray* block_state_arr = cunk_nbt_extract_long_array(block_states);
        int palette_size = palette->body.p_list.count;

        bool is_air[palette_size];
        for (size_t j = 0; j < palette_size; j++) {
            const NBT_Compound* color = &palette->body.p_list.bodies[j].p_compound;
            const char* name = cunk_nbt_compound_direct_access(color, "Name");
            assert(name);
            if (strcmp(name, "minecraft:air") == 0)
                is_air[j] = true;
        }

        int bits = needed_bits(palette_size);
        int longbits = block_state_arr->count * sizeof(int64_t) * CHAR_BIT;
        printf("%d %d %d %d\n", palette_size, bits, longbits, bits * 16 * 16 * 16);
        // cunk_print_nbt(p, block_states);
    }

    return 0;
}
