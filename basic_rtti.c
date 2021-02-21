// Basic RTTI.c, include once, work forever ish


#define MAX_STRUCTURE_INTROSPECTION_INFORMATION_FIELDS 48
#define MAX_TYPE_INFORMATION_TABLE_TYPE_INFORMATIONS 128
#define MAX_TYPE_INFORMATION_TABLE_STRUCTURE_INFORMATIONS 128

#define MAX_IDENTIFIER_NAME_LENGTH 48
#define CHECKING_FOR_UNSIGNED 0 /* I don't think it's too important to know for this test */

#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stdint.h>

#include <string.h>

enum Type_Information_Type {
    TYPE_INFORMATION_BUILT_IN,
    TYPE_INFORMATION_STRUCTURE
};

// index+1
// Only handles are typedef structed, or otherwise structs for "stronger" typing.
typedef struct Structure_Introspection_Information_Handle {
    uint16_t handle;
} Structure_Introspection_Information_Handle;
typedef struct Type_Information_Handle {
    uint16_t handle;
} Type_Information_Handle;

struct Type_Information {
    /*
     * 
    TYPE_INFORMATION_BUILT_IN, --> int, float, double, bool
    TYPE_INFORMATION_STRUCTURE --> struct
    */
    int type;
    char name[MAX_IDENTIFIER_NAME_LENGTH];
    Structure_Introspection_Information_Handle structure_handle;

    size_t size; // sizeof(type)
};

enum Type_Information_Type_Id_Class {
    TYPE_INFORMATION_TYPE_ID_STRUCTURE,
    TYPE_INFORMATION_TYPE_ID_CHARACTER,
    TYPE_INFORMATION_TYPE_ID_INTEGER,
    TYPE_INFORMATION_TYPE_ID_FLOATING_POINT,
    TYPE_INFORMATION_TYPE_ID_BOOLEAN,
    TYPE_INFORMATION_TYPE_ID_COUNT
};

struct Type_Information_Type_Id {
#if CHECKING_FOR_UNSIGNED == 1
    bool is_unsigned;
#endif
    uint8_t bits;
    uint8_t class;
};

struct Structure_Field_Information {
    char name[MAX_IDENTIFIER_NAME_LENGTH];
    size_t offset;

    Type_Information_Handle type_information_handle;

    size_t pointer_depth;
    size_t array_count;
};

struct Structure_Introspection_Information {
    char name[MAX_IDENTIFIER_NAME_LENGTH];

    size_t field_count;
    struct Structure_Field_Information fields[MAX_STRUCTURE_INTROSPECTION_INFORMATION_FIELDS];
};

// structures count as typeinfo
struct Type_Information_Table {
    size_t type_count;
    struct Type_Information types[MAX_TYPE_INFORMATION_TABLE_TYPE_INFORMATIONS];

    size_t structure_count;
    struct Structure_Introspection_Information structures[MAX_TYPE_INFORMATION_TABLE_STRUCTURE_INFORMATIONS];
};
static size_t structure_introspection_information_calculate_size_of(struct Type_Information_Table* table, struct Structure_Introspection_Information* information);

struct Type_Information* type_information_table_register_type(struct Type_Information_Table* table, char* name, size_t size) {
    struct Type_Information* current_type = NULL;
    int already_registered = 0;

    for (size_t type_index = 0; type_index < table->type_count; ++type_index) {
        if (strncmp(name, table->types[type_index].name, strlen(table->types[type_index].name)) == 0) {
            already_registered = 1;
            current_type = &table->types[type_index];
            break;
        }
    }

    if (!already_registered) {
        current_type = &table->types[table->type_count++];
    }

    current_type->type = TYPE_INFORMATION_BUILT_IN;
    {
        strncpy(current_type->name, name, MAX_IDENTIFIER_NAME_LENGTH-1);
    }
    current_type->size = size;

    printf("Registered %s?\n", current_type->name);
    return current_type;
}

// for now this is hardcoded based off of general C standard library types.
// This should probably allow manual registration...
// does not detect unsignedness because I didn't deem it necessary.
// You should probably make your own version of this. The struct is relatively trivial to use.
struct Type_Information_Type_Id type_information_type_id(struct Type_Information* type_information) {
    struct Type_Information_Type_Id type_id = {};

#define string_equal(a, b) (strcmp((a), (b)) == 0)
    char* name = type_information->name;
    if (string_equal(name, "int") ||
        string_equal(name, "int32_t") ||
        string_equal(name, "uint32_t")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_INTEGER;
        type_id.bits = 32;
#if CHECKING_FOR_UNSIGNED == 1
        type_id.is_unsigned = name[0] == 'u';
#endif
    } else if (string_equal(name, "long long") ||
               string_equal(name, "int64_t") ||
               string_equal(name, "uint64_t") ||
               string_equal(name, "size_t")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_INTEGER;
        type_id.bits = 64;
#if CHECKING_FOR_UNSIGNED == 1
        type_id.is_unsigned = name[0] == 'u';
#endif
    } else if (string_equal(name, "bool") ||
               string_equal(name, "_Bool")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_BOOLEAN;
        type_id.bits = sizeof(_Bool) * 8;
    } else if (string_equal(name, "int8_t") ||
               string_equal(name, "uint8_t")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_INTEGER;
        type_id.bits = 8; 
#if CHECKING_FOR_UNSIGNED == 1
        type_id.is_unsigned = name[0] == 'u';
#endif
    } else if (string_equal(name, "short") ||
               string_equal(name, "int16_t") ||
               string_equal(name, "uint16_t")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_INTEGER;
        type_id.bits = 16;
#if CHECKING_FOR_UNSIGNED == 1
        type_id.is_unsigned = name[0] == 'u';
#endif
    } else if (string_equal(name, "float")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_FLOATING_POINT;
        type_id.bits = 32;
    } else if (string_equal(name, "double")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_FLOATING_POINT;
        type_id.bits = 64;
    } else if (string_equal(name, "char")) {
        type_id.class = TYPE_INFORMATION_TYPE_ID_CHARACTER;
        type_id.bits = 8; 
    }
#undef string_equal
    return type_id;
}

// Since this is intended to suppliment hotreloadable code in a sense
// you can register the same structure multiple times. Registering the same structure will simply
// update it's information however. It won't actually allocate anything new.
Type_Information_Handle type_information_table_register_structure(struct Type_Information_Table* table, char* name, Structure_Introspection_Information_Handle structure_handle) {
    struct Type_Information* current_type = NULL;
    int already_registered = 0;

    size_t result_handle_index = 0;
    for (size_t type_index = 0; type_index < table->type_count; ++type_index) {
        if (strncmp(name, table->types[type_index].name, strlen(table->types[type_index].name)) == 0) {
            already_registered = 1;
            current_type = &table->types[type_index];
            result_handle_index = type_index;
            break;
        }
    }

    if (!already_registered) {
        current_type = &table->types[table->type_count++];
        result_handle_index = table->type_count - 1;
    }

    current_type->type = TYPE_INFORMATION_STRUCTURE;
    {
        strncpy(current_type->name, name, MAX_IDENTIFIER_NAME_LENGTH-1);
    }

    struct Structure_Introspection_Information* struct_information = &table->structures[structure_handle.handle];
    current_type->size = structure_introspection_information_calculate_size_of(table, struct_information);
    current_type->structure_handle = structure_handle;
    return (Type_Information_Handle) { result_handle_index };
}

void type_information_table_empty_out_all_structures(struct Type_Information_Table* table) {
    for (size_t structure_index = 0; structure_index < table->structure_count; ++structure_index) {
        table->structures[structure_index].field_count = 0;
    }
}

struct Type_Information_Get_Type_Result {
    Type_Information_Handle type_information_handle;
    size_t constant_array_size;

    uint8_t pointer_indirections;
    uint8_t is_array;
};

struct Type_Information_Get_Type_Result type_information_table_get_type(struct Type_Information_Table* table, char* name);

static void structure_introspection_information_push_field(struct Type_Information_Table* table, struct Structure_Introspection_Information* information, char* field_name, const char* typename_string) {
    struct Structure_Field_Information* current_field = &information->fields[information->field_count++];
    {
        strncpy(current_field->name, field_name, MAX_IDENTIFIER_NAME_LENGTH-1);
    }

    struct Type_Information_Get_Type_Result type_info = type_information_table_get_type(table, typename_string);
    current_field->type_information_handle = type_info.type_information_handle;
    current_field->pointer_depth = type_info.pointer_indirections;
    current_field->array_count = type_info.constant_array_size;
}

static size_t structure_field_information_size(struct Type_Information_Table* table, struct Structure_Field_Information* field) {
    size_t size_to_use = 0;

    if (field->pointer_depth) {
        size_to_use = sizeof(void*);
    } else {
        struct Type_Information* type_information = &table->types[field->type_information_handle.handle];
        size_to_use = type_information->size; 
    }

    return size_to_use * ((field->array_count > 0) ? (field->array_count) : (1));
}

// I can probably always rely on a power of two to appear, but I'll use the general case
size_t aligned_to_boundary(const size_t base, const size_t alignment) {
    return ((base + (alignment-1)) / alignment) * alignment;
}

size_t next_boundary(const size_t base, const size_t alignment) {
    return ((base + (alignment)) / alignment) * alignment;
}

static size_t structure_introspection_information_alignment_of_largest_field(struct Type_Information_Table* table, struct Structure_Introspection_Information* structure_information) {
    size_t largest_member_size = 0;
    for (size_t field_index = 0; field_index < structure_information->field_count; ++field_index) {
        size_t size_of_current_field = 0;

        struct Type_Information* type_information = &table->types[structure_information->fields[field_index].type_information_handle.handle];
        if (type_information->type == TYPE_INFORMATION_STRUCTURE) {
            size_of_current_field = structure_introspection_information_alignment_of_largest_field(table, &table->structures[type_information->structure_handle.handle]);
        } else {
            size_of_current_field = structure_field_information_size(table, &structure_information->fields[field_index]);
        }

        if (structure_information->fields[field_index].array_count) {
            size_of_current_field /= structure_information->fields[field_index].array_count;
        }

        if (largest_member_size < size_of_current_field) {
            largest_member_size = size_of_current_field;
        }
    }

    return largest_member_size;
}

// This is ONLY tested to work with the normal C alignment rules.
// I would hope that this code works with forced packing alignment, (presumably by forcing largest_member_size to be the forced alignment)
static void structure_introspection_information_finalize_entries(struct Type_Information_Table* table, struct Structure_Introspection_Information* information_tables, size_t entries_in_table) {
    for (size_t table_index = 0; table_index < entries_in_table; ++table_index) {
        struct Structure_Introspection_Information* current_table = &information_tables[table_index];

        size_t current_memory_offset = 0;
        size_t largest_member_alignment = structure_introspection_information_alignment_of_largest_field(table, current_table);

        if (largest_member_alignment > 0) {
            for (size_t field_index = 0; field_index < current_table->field_count; ++field_index) {
                size_t field_size = structure_field_information_size(table, &current_table->fields[field_index]);
                current_table->fields[field_index].offset = current_memory_offset;

                struct Structure_Field_Information* next_field = NULL;
                if (!(field_index + 1 >= current_table->field_count)) {
                    next_field = &current_table->fields[field_index+1];
                }

                current_memory_offset += field_size;
                if (next_field) {
                    size_t next_size = structure_field_information_size(table, next_field);
                    size_t remaining_until_boundary = aligned_to_boundary(current_memory_offset,
                                                                          (largest_member_alignment > sizeof(void*)) ? sizeof(void*) : largest_member_alignment) - current_memory_offset;

                    if (next_size > remaining_until_boundary) {
                        current_memory_offset += remaining_until_boundary;
                    } else {
                        // Not sure why this has to special cased, since size doesn't seem to be affected by this.
                        if (next_size > 1) {
                            current_memory_offset += (remaining_until_boundary - next_size);
                        }
                    }
                }
            }
        }
    }
}

static size_t structure_introspection_information_calculate_size_of(struct Type_Information_Table* table, struct Structure_Introspection_Information* information) {
    size_t total_size = 0;

    size_t largest_member_size = structure_introspection_information_alignment_of_largest_field(table, information);
    if (largest_member_size > 0) {
        for (size_t field_index = 0; field_index < information->field_count; ++field_index) {
            struct Type_Information* type_information = &table->types[information->fields[field_index].type_information_handle.handle];

            if (type_information->type == TYPE_INFORMATION_STRUCTURE) {
                type_information_table_register_structure(table, type_information->name, type_information->structure_handle);
            }
            size_t field_size = structure_field_information_size(table, &information->fields[field_index]);

            struct Structure_Field_Information* next_field = NULL;
            if (!(field_index + 1 >= information->field_count)) {
                next_field = &information->fields[field_index+1];
            }

            total_size += field_size;
            if (next_field) {
                size_t next_size = structure_field_information_size(table, next_field);
                size_t remaining_until_boundary =  aligned_to_boundary(total_size, largest_member_size) - total_size;

                if (next_size > remaining_until_boundary) {
                    total_size += remaining_until_boundary;
                } else {
                    total_size += (remaining_until_boundary - next_size);
                }
            }
        }

        if (total_size == aligned_to_boundary(total_size, largest_member_size)) {
            return total_size;
        } else {
            return next_boundary(total_size, largest_member_size);
        }
    } else {
        return 0;
    }
}

Structure_Introspection_Information_Handle type_information_table_push_new_structure(struct Type_Information_Table* table, char* name) {
    int already_exists = 0;
    struct Structure_Introspection_Information* result = NULL;

    size_t structure_index = 0;
    for (structure_index = 0; structure_index < table->structure_count; ++structure_index) {
        if (strncmp(name, table->structures[structure_index].name, strlen(table->structures[structure_index].name)) == 0) {
            already_exists = 1;
            result = &table->structures[structure_index];
            break;
        }
    }

    if (!already_exists) {
        result = &table->structures[table->structure_count++];
        strncpy(result->name, name, MAX_IDENTIFIER_NAME_LENGTH-1);
        structure_index = table->structure_count-1;
    }

    type_information_table_register_structure(table, name, (Structure_Introspection_Information_Handle){structure_index});
    return (Structure_Introspection_Information_Handle){structure_index};
}

// This function is pretty bad, and should be implemented differently.
// For one not parsing the type string here, and being split into separate functions.
struct Type_Information_Get_Type_Result type_information_table_get_type(struct Type_Information_Table* table, char* name) {
    int asterisks = 0;
    int array_count = 0;

    // stupid temporary buffer
    // ignorant assumption that strings are null terminated!
    // I do not attempt to fail parsing atm cause I'm lazy
    size_t length_of_actual_type_name = 0;
    {
        for (size_t character_index = 0; character_index < strlen(name); ++character_index) {
            switch (name[character_index]) {
                case '\0': {} break;
                case '*': {
                    asterisks++;
                } break;
                case '[':
                {
                    size_t end_of_brackets = character_index;
                    character_index++;
                    while (name[end_of_brackets + 1] != ']'){end_of_brackets++;};

                    size_t digit_count = end_of_brackets - character_index + 1;
                    size_t accumulator = 0;
                    for (size_t digit_index = 0; digit_index < digit_count; ++digit_index) {
                        int power_of_ten = (int)pow(10, (digit_count - 1) - digit_index);
                        accumulator += power_of_ten * (name[digit_index + character_index] - '0');
                    }

                    array_count = accumulator;
                    character_index = end_of_brackets + 1;
                }
                break;
                default:
                {
                    length_of_actual_type_name++;
                }
                break;
            }
        }
    }

    for (size_t type_index = 0; type_index < table->type_count; ++type_index) {
        if (strncmp(name, table->types[type_index].name, strlen(table->types[type_index].name)) == 0) {
            return (struct Type_Information_Get_Type_Result) {
                .constant_array_size = array_count,
                    .pointer_indirections = asterisks,
                    .is_array = array_count > 0,
                    .type_information_handle = (Type_Information_Handle){type_index}
            };
        }
    }

    char actual_type_name[1024];
    strncpy(actual_type_name, name, length_of_actual_type_name);
    actual_type_name[length_of_actual_type_name] = 0;
    return (struct Type_Information_Get_Type_Result) {
        .constant_array_size = array_count,
            .pointer_indirections = asterisks,
            .is_array = array_count > 0,
            .type_information_handle = type_information_table_register_structure(table, actual_type_name, type_information_table_push_new_structure(table, actual_type_name))};
}

static void type_information_table_finalize_structures(struct Type_Information_Table* table) {
    for (size_t type_index = 0; type_index < table->type_count; ++type_index) {
        struct Type_Information* type_information = &table->types[type_index];
        if (type_information->type == TYPE_INFORMATION_STRUCTURE) {
            type_information_table_register_structure(table, type_information->name, type_information->structure_handle);
        }
    }
    structure_introspection_information_finalize_entries(table, table->structures, table->structure_count);
}
