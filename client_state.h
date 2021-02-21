#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include "game_stuff.h"

struct Client_State {
    uint32_t window_resolution_w;
    uint32_t window_resolution_h;

    struct Character_Fixed_Size_Array* characters;

    TTF_Font* default_font;
    struct Type_Information_Table* runtime_type_information_table;
    struct Type_Information_Table* previous_runtime_type_information_table;

    int first_load;
};
#endif
