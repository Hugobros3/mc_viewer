#include "nbt.h"

#include "cunk/print.h"

#pragma GCC diagnostic error "-Wswitch"

static const char* nbt_tag_name[] = {
"End",
#define T(name, _) #name,
NBT_TAG_TYPES(T)
#undef T
};

static void print_nbt_body(Printer* p, NBT_Tag tag, NBT_Body body) {
    switch (tag) {
        case NBT_Tag_End:    cunk_print(p, "END"); return;
        case NBT_Tag_Byte:   cunk_print(p, "%d", body.p_byte); break;
        case NBT_Tag_Short:  cunk_print(p, "%d", body.p_short); break;
        case NBT_Tag_Int:    cunk_print(p, "%d", body.p_int); break;
        case NBT_Tag_Long:   cunk_print(p, "%d", body.p_long); break;
        case NBT_Tag_Float:  cunk_print(p, "%f", body.p_float); break;
        case NBT_Tag_Double: cunk_print(p, "%f", body.p_double); break;
        case NBT_Tag_ByteArray:
            cunk_print(p, "[");
            for (int32_t i = 0; i < body.p_byte_array.count; i++) {
                cunk_print(p, "%d", body.p_byte_array.arr[i]);
                if (i + 1 < body.p_byte_array.count)
                    cunk_print(p, ", ");
            }
            cunk_print(p, "]");
            break;
        case NBT_Tag_String:
            cunk_print(p, "\"%s\"", body.p_string);
            break;
        case NBT_Tag_List:
            cunk_print(p, "(%s[]) [", nbt_tag_name[body.p_list.tag]);
            for (int32_t i = 0; i < body.p_list.count; i++) {
                print_nbt_body(p, body.p_list.tag, body.p_list.bodies[i]);
                if (i + 1 < body.p_list.count)
                    cunk_print(p, ", ");
            }
            cunk_print(p, "]");
            break;
        case NBT_Tag_Compound:
            cunk_print(p, "{");
            cunk_indent(p);
            cunk_newline(p);
            for (int32_t i = 0; i < body.p_compound.count; i++) {
                cunk_print_nbt(p, body.p_compound.objects[i]);
                if (i + 1 < body.p_compound.count)
                    cunk_newline(p);
            }
            cunk_deindent(p);
            cunk_newline(p);
            cunk_print(p, "}");
            break;
        case NBT_Tag_IntArray:
            cunk_print(p, "[");
            for (int32_t i = 0; i < body.p_int_array.count; i++) {
                cunk_print(p, "%d", body.p_int_array.arr[i]);
                if (i + 1 < body.p_int_array.count)
                    cunk_print(p, ", ");
            }
            cunk_print(p, "]");
            break;
        case NBT_Tag_LongArray:
            cunk_print(p, "[");
            for (int32_t i = 0; i < body.p_long_array.count; i++) {
                cunk_print(p, "%d", body.p_long_array.arr[i]);
                if (i + 1 < body.p_long_array.count)
                    cunk_print(p, ", ");
            }
            cunk_print(p, "]");
            break;
    }
}

void cunk_print_nbt(Printer* p, const NBT_Object* o) {
    cunk_print(p, "%s %s = ", nbt_tag_name[o->tag], o->name);
    print_nbt_body(p, o->tag, o->body);
}
