#include "nbt.h"
#include "zlib_wrap.h"

#include "cunk/memory.h"
#include "cunk/print.h"
#include "cunk/io.h"
#include "cunk/util.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
    Printer* p = cunk_open_file_as_printer(stdout);

    char* buf;
    size_t buf_size;
    printf("Using test file %s\n", argv[1]);
    if (!read_file(argv[1], &buf_size, &buf))
        return 1;

    if (string_ends_with(argv[1], ".dat")) {
        printf(".dat file detected, it needs decompression\n");
        printf("compressed size: %llu\n", buf_size);
        Growy* g = cunk_new_growy();
        cunk_inflate(ZLib_GZip, buf_size, buf, g);
        free(buf);
        buf_size = cunk_growy_size(g);
        buf = cunk_growy_deconstruct(g);
        printf("Decompression successful\n");
    }

    Arena* arena = cunk_new_arena();
    NBT_Object* o = cunk_decode_nbt(buf_size, buf, arena);
    assert(o);
    cunk_print_nbt(p, o);
    cunk_print(p, "\nSize of NBT arena: ");
    cunk_print_size_suffix(p, cunk_arena_size(arena), 3);
    cunk_print(p, "\n");
    free(buf);
    cunk_arena_destroy(arena);
    return 0;
}
