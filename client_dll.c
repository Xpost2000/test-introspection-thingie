#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "basic_rtti.c"
#include "client_state.h"

static float scroll_y = 0; // defparameter?

void client_on_reload(struct Client_State* state) {
    printf("On reload\n");

    // we can't remove types, but we can liberally change them
    type_information_table_empty_out_all_structures(state->runtime_type_information_table);
    {
        Structure_Introspection_Information_Handle handle = type_information_table_push_new_structure(state->runtime_type_information_table, "Character_Fixed_Size_Array");
        struct Structure_Introspection_Information* current_introspection_information_entry = &state->runtime_type_information_table->structures[handle.handle];
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "count", "size_t");
#ifdef TEST
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "magikun", "size_t");
#endif
#ifdef NEXT
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "magikun_potato", "size_t");
#endif
#ifdef FINAL_TEST
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "characters", "Character[3]");
#endif
    }
    {
        Structure_Introspection_Information_Handle handle = type_information_table_push_new_structure(state->runtime_type_information_table, "Character");
        struct Structure_Introspection_Information* current_introspection_information_entry = &state->runtime_type_information_table->structures[handle.handle];
#ifdef DYNAMIC_CHARACTER_POINTER
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "name", "char*");
#else
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "name", "char[64]");
#endif
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "level", "uint32_t");
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "health", "uint32_t");
#ifdef SECOND_EDITION
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "stamina", "int64_t");
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "mana", "uint16_t");
#endif
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "defense", "uint32_t");
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "speed", "double");
#ifdef USE_INVENTORY
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "inventory", "Item[10]");
#endif
    }
    {
        Structure_Introspection_Information_Handle handle = type_information_table_push_new_structure(state->runtime_type_information_table, "Item");
        struct Structure_Introspection_Information* current_introspection_information_entry = &state->runtime_type_information_table->structures[handle.handle];
#ifdef DYNAMIC_CHARACTER_POINTER
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "name", "char*");
#else
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "name", "char[64]");
#endif    
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "type", "int32_t");
#ifdef TUNA_FISH
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "TUNA", "int64_t");
#endif
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "magnitude", "int32_t");
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "cost", "int32_t");
        structure_introspection_information_push_field(state->runtime_type_information_table, current_introspection_information_entry, "weight", "int32_t");
    }
    type_information_table_finalize_structures(state->runtime_type_information_table);

#if 1
    if (!state->first_load) {
        state->first_load = true;
        printf("first reload!\n");
    } else {
        int engage_monkey_patch = 0;
        {
            // checking for structure differences...
            struct Type_Information_Table* current_table = state->runtime_type_information_table;
            struct Type_Information_Table* previous_table = state->previous_runtime_type_information_table;

            if (current_table->type_count != previous_table->type_count ||
                current_table->structure_count != previous_table->structure_count) {
                engage_monkey_patch = 1;
                printf("type count difference! Monkey patch engaging!\n");
            } else {
                // one by one check of structure fields and such.
                // a hash comparison would be nice.
                for (size_t structure_index = 0; structure_index < previous_table->structure_count; ++structure_index) {
                    struct Structure_Introspection_Information* current_version_of_structure = &current_table->structures[structure_index];
                    struct Structure_Introspection_Information* previous_version_of_structure = &previous_table->structures[structure_index];

                    if (current_version_of_structure->field_count != previous_version_of_structure->field_count) {
                        engage_monkey_patch = 1; 
                        break;
                    }
                }
            }
        }

        // Basically, identify the entire region I have to monkey patch
        // (in this case a "game state" pointer. Client state isn't the game state btw)

        /*
         * This monkey patch is basically just treated as updating a tree/graph from a root.
         * Basically the big struct we're hotreloading is assumed to be a root node. Any triggered
         * type differences will update correctly for the most part.
         */

        // in this case that's the characters thing
        // This memory should be in separate region that is probably heap allocated!
        if (engage_monkey_patch) {
            struct Type_Information* structure_to_monkey_patch_type_information = &state->runtime_type_information_table->types[type_information_table_get_type(state->runtime_type_information_table, "Character_Fixed_Size_Array").type_information_handle.handle];
            struct Type_Information* previous_structure_to_monkey_patch_type_information = &state->previous_runtime_type_information_table->types[type_information_table_get_type(state->previous_runtime_type_information_table, "Character_Fixed_Size_Array").type_information_handle.handle];

            struct Structure_Introspection_Information* structure_to_monkey_patch = &state->runtime_type_information_table->structures[structure_to_monkey_patch_type_information->structure_handle.handle];
            struct Structure_Introspection_Information* previous_structure_to_monkey_patch = &state->previous_runtime_type_information_table->structures[previous_structure_to_monkey_patch_type_information->structure_handle.handle];

            printf("(current)%s: %d\n", structure_to_monkey_patch_type_information->name, structure_to_monkey_patch_type_information->size);
            printf("(last)%s: %d\n", previous_structure_to_monkey_patch_type_information->name, previous_structure_to_monkey_patch_type_information->size);

            // Probably no memory leak, although this assumes my characters are allocated from malloc/free.
            // optimally the host provides ways to do allocation
            void* last_memory_copy = state->characters;
            void* new_memory_copy = malloc(structure_to_monkey_patch_type_information->size);
            memset(new_memory_copy, 0, structure_to_monkey_patch_type_information->size);

            for (size_t field_index = 0; field_index < structure_to_monkey_patch->field_count; ++field_index) {
                struct Structure_Field_Information* current_field = &structure_to_monkey_patch->fields[field_index]; 
                struct Structure_Field_Information* same_field_in_previous_version = NULL;
                printf("%s looking for field in old rtti!\n\n", current_field->name);

                // searching for similar member.
                // (This part breaks pointers since variables are allowed to switch types and offset...)
                // (try to avoid having pointers to anything that is being hotreloaded.)
                for (size_t old_field_index = 0; old_field_index < previous_structure_to_monkey_patch->field_count; ++old_field_index) {
                    struct Structure_Field_Information* old_current_field = &previous_structure_to_monkey_patch->fields[old_field_index];
                    if (strcmp(old_current_field->name, current_field->name) == 0) {
                        same_field_in_previous_version = old_current_field;
                        printf("(%s(%d) vs %s(%d)), Same field was found! Going to copy over to new\n\n", old_current_field->name, old_current_field->offset, current_field->name, current_field->offset);
                        break;
                    }
                }

                void* base_pointer_for_new_field = new_memory_copy + current_field->offset;
                size_t size_of_current = structure_field_information_size(state->runtime_type_information_table, current_field);
                if (same_field_in_previous_version) {
                    void* base_pointer_for_previous_field = last_memory_copy + same_field_in_previous_version->offset;

                    size_t size_of_last = structure_field_information_size(state->previous_runtime_type_information_table, same_field_in_previous_version);
                    size_t smaller_memory_size;
                    {
                        smaller_memory_size = (size_of_last < size_of_current) ? size_of_last : size_of_current;
                    }

                    memcpy(base_pointer_for_new_field,
                           base_pointer_for_previous_field,
                           smaller_memory_size);
                } else {
                    printf("This is a new field...What do I do?\n");
                    memset(base_pointer_for_new_field, 0, size_of_current);
                }
            }

            state->characters = (struct Character_Fixed_Size_Array*)new_memory_copy;
            free(last_memory_copy);
        }
    }
#endif
    printf("finish reload\n");
}

void client_on_unload(struct Client_State* state) {
    printf("On unload\n");
    /* memcpy(state->previous_runtime_type_information_table, state->runtime_type_information_table, sizeof(*state->runtime_type_information_table)); */
    *state->previous_runtime_type_information_table = *state->runtime_type_information_table;
}

void client_on_keydown(struct Client_State* state, uint32_t keysym) {
    /* printf("client key down\n"); */
    if (keysym == SDLK_UP) {
        scroll_y += 5;
    }

    if (keysym == SDLK_DOWN) {
        scroll_y -= 5;
    }
}

void client_on_keyup(struct Client_State* state, uint32_t keysym) {
    /* printf("client key up\n"); */
}

void client_on_mouse_event(struct Client_State* state, uint32_t x, uint32_t y, int32_t button) {
    /* printf("client mouse event\n"); */
}

void client_on_resolution_change(struct Client_State* state, uint32_t new_width, uint32_t new_height) {
    printf("client resolution change\n");
}

void client_on_quit(struct Client_State* state) {
    printf("client quit\n");
}

void client_on_update(struct Client_State* state, float delta_time) {
    /* printf("client update\n"); */
}

static void temporary_draw_text(SDL_Renderer* renderer,
                                TTF_Font* font,
                                const char* string,
                                float x, float y,
                                float scale,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (strlen(string) <= 0 || !string) return;

    SDL_Surface* temporary_text_surface = TTF_RenderText_Blended(font, string, (SDL_Color){r,g,b,a});
    SDL_Texture* as_texture = SDL_CreateTextureFromSurface(renderer, temporary_text_surface);
    SDL_Rect destination_rect = {(int)x, (int)y, temporary_text_surface->w * (int)scale, temporary_text_surface->h * (int)scale};

    SDL_RenderCopy(renderer, 
                   as_texture,
                   NULL,
                   &destination_rect);

    SDL_FreeSurface(temporary_text_surface);
    SDL_DestroyTexture(as_texture);
}

#if 1
static float temporary_drawing_do_basic_type_introspection_on_pointer(SDL_Renderer* renderer, TTF_Font* font, float x, float y, void* pointer, struct Type_Information_Table* table, Type_Information_Handle type_information_handle) {
    void* inspected_pointer = pointer;

    float cursor_x = x;
    float cursor_y = y;

    struct Type_Information* type_information = &table->types[type_information_handle.handle];
    struct Structure_Introspection_Information* structure_pointer = &table->structures[type_information->structure_handle.handle];
    for (size_t field_index = 0; field_index < structure_pointer->field_count; ++field_index) {
        struct Structure_Field_Information* current_field = &structure_pointer->fields[field_index];
        struct Type_Information* current_field_type_information = &table->types[current_field->type_information_handle.handle];

        char temporary_buffer[1024] = {};
        int offset_x = 0;
        {
            offset_x = snprintf(temporary_buffer, 1024, "%s (offset %d): ", current_field->name, current_field->offset);
            temporary_draw_text(renderer, font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255);
            cursor_x += offset_x * 8;
        }

        struct Type_Information_Type_Id type_id = type_information_type_id(current_field_type_information);
        if (type_id.class == TYPE_INFORMATION_TYPE_ID_CHARACTER) {
            // since characters can be strings. 
            void* base_pointer = inspected_pointer + current_field->offset;
            size_t* indirected_pointer = NULL;

            // This will follow as many pointers as we can, leaving only the last pointer.
            if (current_field->pointer_depth) {
                size_t* current_pointer = base_pointer;
                for (size_t pointer_depth = 0; pointer_depth < current_field->pointer_depth; ++pointer_depth) {
                    size_t new_pointer = *current_pointer;
                    current_pointer = (size_t*)new_pointer;
                }
                indirected_pointer = current_pointer;
            }

            /* printf("%d\n", current_field->array_count); */
            if (current_field->pointer_depth || current_field->array_count) {
                if (current_field->pointer_depth) {
                    snprintf(temporary_buffer, 1024, "(ptr)%s", (char*)indirected_pointer);
                } else {
                    snprintf(temporary_buffer, 1024, "(fixed size string)%s", (char*)base_pointer);
                }
            } else {
                snprintf(temporary_buffer, 1024, "(char)%c", *((char*)base_pointer));
            }
            temporary_draw_text(renderer, font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255);
            cursor_y += 16;
        } else {
            for (size_t array_counter_index = 0; array_counter_index < ((current_field->array_count > 1) ? current_field->array_count-1 : 1); ++array_counter_index) {
                void* base_pointer = inspected_pointer + current_field->offset + (current_field_type_information->size * array_counter_index);
                size_t* indirected_pointer = NULL;

                if (current_field->pointer_depth) {
                    size_t* current_pointer = base_pointer;
                    for (size_t pointer_depth = 0; pointer_depth < current_field->pointer_depth; ++pointer_depth) {
                        size_t new_pointer = *current_pointer;
                        current_pointer = (size_t*)new_pointer;
                    }
                    indirected_pointer = current_pointer;
                }

                switch (type_id.class) {
                    case TYPE_INFORMATION_TYPE_ID_INTEGER:
                    {
                        if (type_id.bits == 64) {
                            snprintf(temporary_buffer, 1024, "(i64)%d", (*(int64_t*)(base_pointer)));
                        } else if (type_id.bits == 32) {
                            snprintf(temporary_buffer, 1024, "(i32)%d", (*(int32_t*)(base_pointer)));
                        }
                        temporary_draw_text(renderer, font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255);
                        cursor_y += 16;
                    }
                    break;
                    case TYPE_INFORMATION_TYPE_ID_FLOATING_POINT:
                    {
                        if (type_id.bits == 64) {
                            snprintf(temporary_buffer, 1024, "(double)%3.3f", *((double*)base_pointer));
                        } else if (type_id.bits == 32) {
                            snprintf(temporary_buffer, 1024, "(float)%3.3f", *((double*)base_pointer));
                        }
                        temporary_draw_text(renderer, font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255);
                        cursor_y += 16;
                    }
                    break;
                    case TYPE_INFORMATION_TYPE_ID_STRUCTURE:
                    {
                        if (indirected_pointer) {
                            /* printf("Unsure of how to handle pointer indirection on unknown type.\n"); */
                        } else {
                            char temporary_buffer[1024] = {};
                            {
                                cursor_y += 16;
                                snprintf(temporary_buffer, 1024, "SUB TYPE(%s)", current_field_type_information->name);
                                temporary_draw_text(renderer, font, temporary_buffer, cursor_x - offset_x * 6, cursor_y, 1, 0, 255, 255, 255);
                                cursor_y += 16;
                            }

                            cursor_y = temporary_drawing_do_basic_type_introspection_on_pointer(renderer,
                                                                                                font,
                                                                                                cursor_x - (offset_x * 6) + 8 * 4,
                                                                                                cursor_y,
                                                                                                base_pointer, table, current_field->type_information_handle);
                        }
                    }
                    break;
                    case TYPE_INFORMATION_TYPE_ID_BOOLEAN:
                    {
                        int value = *((int*)base_pointer);
                        {
                            snprintf(temporary_buffer, 1024, "(boolean)");
                            if (value) {
                                snprintf(temporary_buffer, 1024, "true");
                            } else{
                                snprintf(temporary_buffer, 1024, "false");
                            }
                        }
                        temporary_draw_text(renderer, font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255);
                        cursor_y += 16;
                    }
                    break;
                    default: {} break;
                }
            }
        }

        cursor_x -= offset_x * 8;
    }
    return cursor_y;
}
#endif

void client_on_draw(struct Client_State* state, SDL_Renderer* renderer) {
    float new_color = (sin(SDL_GetTicks() / 1000.0) + 1) * 100;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    {
        SDL_SetRenderDrawColor(renderer, new_color, 0, 0, new_color);
        SDL_RenderFillRect(renderer, &(SDL_Rect) {128, 150, 255, 255});
    }

    char temporary_buffer[1024] = {};

#if 0
    {
        float cursor_x = 300;
        float cursor_y = 0;
        for (size_t structure_index = 0; structure_index < state->runtime_type_information_table->structure_count; ++structure_index) {
            struct Structure_Introspection_Information* current_structure = &state->runtime_type_information_table->structures[structure_index];
            {
                snprintf(temporary_buffer, 1024, "STRUCT %s", current_structure->name);
                temporary_draw_text(renderer, state->default_font, temporary_buffer, cursor_x, cursor_y, 1, 255, 255, 255, 255);
                cursor_y += 16;
                cursor_x += 8*4;
            }
            for (size_t field_index = 0; field_index < current_structure->field_count; ++field_index) {
                {
                    snprintf(temporary_buffer, 1024, "field: %s, offset: %d (next +%d)",
                             current_structure->fields[field_index].name,
                             current_structure->fields[field_index].offset,
                             structure_field_information_size(state->runtime_type_information_table, &current_structure->fields[field_index])
                    );
                    temporary_draw_text(renderer, state->default_font, temporary_buffer, cursor_x, cursor_y, 1, 255, 255, 255, 255);
                    cursor_y += 16;
                }
            }
            cursor_x -= 8*4;
        }
    }
#endif

    /* { */
    /*     float cursor_x = 300; */
    /*     float cursor_y = 265; */
        
    /*     for (size_t character_index = 0; character_index < state->characters.count; ++character_index) { */
    /*         struct Character* current_character = &state->characters.characters[character_index]; */
    /*         snprintf(temporary_buffer, 255, "%d: %s", character_index, current_character->name); */
    /*         temporary_draw_text(renderer, state->default_font, temporary_buffer, cursor_x, cursor_y, 1, 0, 255, 255, 255); */
    /*         cursor_y += 16; */
    /*     } */
    /* } */

#if 0
    {
        temporary_drawing_do_basic_type_introspection_on_pointer(renderer,
                                                                 state->default_font,
                                                                 0, scroll_y, state->characters,
                                                                 state->runtime_type_information_table,
                                                                 type_information_table_get_type(state->runtime_type_information_table, "Character_Fixed_Size_Array")
                                                                 .type_information_handle);
    }
    /* printf("%d\n", state->characters->characters[0].health); */
#endif
#ifdef TEST
    state->characters->magikun = 92;
    /* state->characters->characters[0].name = NULL; */
#endif
#ifdef TUNA_FISH
    state->characters->characters[0].inventory[0].magnitude = 1249;
    state->characters->characters[0].inventory[0].type = 4;
    state->characters->characters[0].inventory[0].TUNA = 9999;

    state->characters->characters[0].inventory[1].magnitude = 1249;
    state->characters->characters[0].inventory[1].type = 8;
    state->characters->characters[0].inventory[1].TUNA = 922;
#endif
/* printf("%d\n", state->characters->count); */

    state->characters->magikun_potato = 4;
    state->characters->magikun = 9;
/* printf("%d, %d\n", sizeof(struct Character_Fixed_Size_Array), sizeof(struct Character)); */
    SDL_RenderPresent(renderer);
}
