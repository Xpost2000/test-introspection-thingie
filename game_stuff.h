#ifndef GAME_STUFF_H
#define GAME_STUFF_H
#define FINAL_TEST
#define USE_INVENTORY
#define TEST
#define NEXT
#define SECOND_EDITION
/* #define DYNAMIC_CHARACTER_POINTER */
#define TUNA_FISH

enum Item_Type {
    ITEM_TYPE_NONE,
    ITEM_TYPE_MISC,
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_ARMOR,
    ITEM_TYPE_COUNT
};

struct Item {
#ifdef DYNAMIC_CHARACTER_POINTER
    char* name;
#else
    char name[64];
#endif
    int32_t type;
#ifdef TUNA_FISH
    int64_t TUNA;
#endif
    int32_t magnitude;
    int32_t cost;
    int32_t weight;
};

struct Character {
#ifdef DYNAMIC_CHARACTER_POINTER
    char* name;
#else
    char name[64];
#endif
    uint32_t level;
    
    uint32_t health;
#ifdef SECOND_EDITION
    int64_t stamina;
    uint16_t mana;
#endif
    uint32_t defense;

    double speed;
#ifdef USE_INVENTORY
    struct Item inventory[10];
#endif
};

struct Character_Fixed_Size_Array {
    size_t count;
#ifdef TEST
    size_t magikun;
#endif
#ifdef NEXT
    size_t magikun_potato;
#endif
#ifdef FINAL_TEST
    struct Character characters[3];
#endif
};

#endif
