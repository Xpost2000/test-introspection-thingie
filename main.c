// structure padding supported (at least according to base C rules.)
// gcc main.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
#define DEVELOPMENT_BUILD
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "basic_rtti.c"

#include "client_state.h"

#include <Windows.h>

#define SECONDS_BEFORE_HOT_RELOADING_CLIENT_DLL (1)

typedef void (*client_function_pointer_on_reload)(struct Client_State*);
typedef void (*client_function_pointer_on_unload)(struct Client_State*);

typedef void (*client_function_pointer_on_keydown)(struct Client_State*, uint32_t);
typedef void (*client_function_pointer_on_keyup)(struct Client_State*, uint32_t);

typedef void (*client_function_pointer_on_mouse_event)(struct Client_State*, uint32_t, uint32_t, int32_t);
typedef void (*client_function_pointer_on_resolution_change)(struct Client_State*, uint32_t, uint32_t);
typedef void (*client_function_pointer_on_quit)(struct Client_State*);
typedef void (*client_function_pointer_on_update)(struct Client_State*, float);
typedef void (*client_function_pointer_on_draw)(struct Client_State*, SDL_Renderer*);

struct Client_Function_Pointers {
    void* handle_to_shared_object;

    client_function_pointer_on_keydown on_keydown;
    client_function_pointer_on_keyup on_keyup;
    client_function_pointer_on_mouse_event on_mouse_event;
    client_function_pointer_on_resolution_change on_resolution_change;
    client_function_pointer_on_quit on_quit;
    client_function_pointer_on_update on_update;
    client_function_pointer_on_draw on_draw;
    client_function_pointer_on_reload on_reload;
    client_function_pointer_on_unload on_unload;
};

int win32_stat_timestamp_difference(BY_HANDLE_FILE_INFORMATION* first, BY_HANDLE_FILE_INFORMATION* second) {
    typedef union dumb_caster {
        FILETIME file_time;
        unsigned long long number;
    } dumb_caster;

    dumb_caster time_first = {first->ftLastWriteTime};
    dumb_caster time_second = {second->ftLastWriteTime};

    return (time_first.number != time_second.number);
}

void update_win32_file_information(char* file_name, BY_HANDLE_FILE_INFORMATION* information) {
    OFSTRUCT file_structure_dump;
    HANDLE win32_file = (HANDLE)(OpenFile(file_name, &file_structure_dump, OF_READ));
    GetFileInformationByHandle(win32_file, information);
    CloseHandle(win32_file);
}

// TODO(jerry): return value for finding something?
static void client_state_function_pointers_unload(struct Client_Function_Pointers* function_pointers) {
    if (function_pointers->handle_to_shared_object) {
        SDL_UnloadObject(function_pointers->handle_to_shared_object);
        function_pointers->handle_to_shared_object = NULL;
    }
}

static void client_state_function_pointers_update_from(struct Client_Function_Pointers* function_pointers, char* shared_object_name) {
    void* shared_object = SDL_LoadObject(shared_object_name); 
    if (shared_object) {
        if (function_pointers->handle_to_shared_object) {
            SDL_UnloadObject(function_pointers->handle_to_shared_object);
            function_pointers->handle_to_shared_object = NULL;
        }
        function_pointers->handle_to_shared_object = shared_object;
        {
            function_pointers->on_keydown = (client_function_pointer_on_keydown)SDL_LoadFunction(shared_object, "client_on_keydown");
            function_pointers->on_keyup = (client_function_pointer_on_keydown)SDL_LoadFunction(shared_object, "client_on_keydown");;
            function_pointers->on_mouse_event = (client_function_pointer_on_mouse_event)SDL_LoadFunction(shared_object, "client_on_mouse_event");
            function_pointers->on_resolution_change = (client_function_pointer_on_resolution_change)SDL_LoadFunction(shared_object, "client_on_resolution_change");
            function_pointers->on_quit = (client_function_pointer_on_quit)SDL_LoadFunction(shared_object, "client_on_quit");
            function_pointers->on_update = (client_function_pointer_on_update)SDL_LoadFunction(shared_object, "client_on_update");
            function_pointers->on_draw = (client_function_pointer_on_draw)SDL_LoadFunction(shared_object, "client_on_draw");

            function_pointers->on_reload = (client_function_pointer_on_reload)SDL_LoadFunction(shared_object, "client_on_reload");
            function_pointers->on_unload = (client_function_pointer_on_unload)SDL_LoadFunction(shared_object, "client_on_unload");
        }
    } else {
        SDL_UnloadObject(shared_object);
    }
}

static bool win32_file_exists(char* file_name) {
    WIN32_FIND_DATA find_file_data;
    HANDLE file_handle = FindFirstFile(file_name, &find_file_data);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    } else {
        FindClose(file_handle);
        return true;
    }
}

int main(int argc, char** argv) {
    struct Type_Information_Table type_lookup_table = {};
    struct Type_Information_Table previous_type_lookup_table = {};
    {
        // TODO https://www.cplusplus.com/reference/cstdint/
        type_information_table_register_type(&type_lookup_table, "bool", sizeof(bool));
        type_information_table_register_type(&type_lookup_table, "_Bool", sizeof(_Bool));
        type_information_table_register_type(&type_lookup_table, "char", sizeof(char));

        type_information_table_register_type(&type_lookup_table, "long long", sizeof(long long));

        type_information_table_register_type(&type_lookup_table, "int32_t", sizeof(int32_t));
        type_information_table_register_type(&type_lookup_table, "uint32_t", sizeof(uint32_t));

        type_information_table_register_type(&type_lookup_table, "int64_t", sizeof(int64_t));
        type_information_table_register_type(&type_lookup_table, "uint64_t", sizeof(uint64_t));

        type_information_table_register_type(&type_lookup_table, "short", sizeof(short));
        type_information_table_register_type(&type_lookup_table, "int16_t", sizeof(int16_t));
        type_information_table_register_type(&type_lookup_table, "uint16_t", sizeof(uint16_t));

        type_information_table_register_type(&type_lookup_table, "int8_t", sizeof(int8_t));
        type_information_table_register_type(&type_lookup_table, "uint8_t", sizeof(uint8_t));

        type_information_table_register_type(&type_lookup_table, "size_t", sizeof(size_t));

        type_information_table_register_type(&type_lookup_table, "float", sizeof(float));
        type_information_table_register_type(&type_lookup_table, "double", sizeof(double));
    }

    {
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
    }
    uint32_t window_resolution_w = 1024;
    uint32_t window_resolution_h = 768;

    TTF_Font* editor_default_font = TTF_OpenFont("dina.ttf", 16);
    SDL_Window* window = SDL_CreateWindow("TestIntrospector",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          window_resolution_w,
                                          window_resolution_h,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    struct Client_State client_state = {};

    client_state.default_font = editor_default_font;
    client_state.runtime_type_information_table = &type_lookup_table;
    client_state.previous_runtime_type_information_table = &previous_type_lookup_table;

    struct Client_Function_Pointers client_function_pointers = {};
    client_state.characters = malloc(sizeof(*client_state.characters));
    memset(client_state.characters, 0, sizeof(*client_state.characters));
#if 1
    {
        struct Character* character = &client_state.characters->characters[client_state.characters->count++];

#ifdef DYNAMIC_CHARACTER_POINTER
        character->name = "Sarevok";
#else
        strncpy(character->name, "Sarevok", 64);
#endif
        character->level = 15;

        character->health = 120;
        character->defense = 50;
#ifdef SECOND_EDITION
        character->stamina = 245;
        character->mana = 3129;
#endif

        character->speed = 100.0;
        {
            struct Item* item = &character->inventory[0] ;
#ifdef DYNAMIC_CHARACTER_POINTER
            item->name = "Sword of Chaos +2";
#else
            strncpy(item->name, "Sword of Chaos +2", 63);
#endif
            item->type = ITEM_TYPE_WEAPON;
            item->magnitude = 15;
            item->cost = 1000;
            item->weight = 250;
        }
        {
            struct Item* item = &character->inventory[2];
#ifdef DYNAMIC_CHARACTER_POINTER
            item->name = "Claws";
#else
            strncpy(item->name, "Claws", 63);
#endif
            item->type = ITEM_TYPE_WEAPON;
            item->magnitude = 15;
            item->cost = 1000;
            item->weight = 250;
        }
    }
    {
        struct Character* character = &client_state.characters->characters[client_state.characters->count++];
#ifdef DYNAMIC_CHARACTER_POINTER
        character->name = "Bhaal";
#else
        strncpy(character->name, "Bhaal", 63);
#endif
        character->level = 50;

        character->health = 820;
        character->defense = 150;

        character->speed = 250.0;
        {
            struct Item* item = &character->inventory[0];
#ifdef DYNAMIC_CHARACTER_POINTER
            item->name = "Claws";
#else
            strncpy(item->name,"Claws", 63);
#endif
            item->type = ITEM_TYPE_WEAPON;
            item->magnitude = 15;
            item->cost = 1000;
            item->weight = 250;
        }
        {
            struct Item* item = &character->inventory[1];
#ifdef DYNAMIC_CHARACTER_POINTER
            item->name = "Seal";
#else
            strncpy(item->name, "Seal", 63);
#endif
            item->type = ITEM_TYPE_MISC;
            item->magnitude = 0;
            item->cost = 1;
            item->weight = 666;
        }
    }
#endif

    bool running = true;

    uint32_t frame_start_time = 0;
    uint32_t frame_end_time = 0;
    float delta_time = 0.0;

    BY_HANDLE_FILE_INFORMATION last_stat_information = {};
    BY_HANDLE_FILE_INFORMATION current_stat_information = {};
    
    client_state_function_pointers_update_from(&client_function_pointers, "client.dll");
    HANDLE log_file_handle = CreateFileA("out.txt",
                                         FILE_APPEND_DATA,
                                         FILE_SHARE_WRITE | FILE_SHARE_READ,
                                         &(SECURITY_ATTRIBUTES) {.nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = true},
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);
    while (running) {
        frame_start_time = SDL_GetTicks();

#ifdef DEVELOPMENT_BUILD
        {
            static HANDLE compilation_process = NULL;
            static HANDLE process_standard_output = NULL;
            static HANDLE process_standard_error_output = NULL;
            static HANDLE process_standard_input = NULL;

            static DWORD last_known_process_exit_code = -1;

            static float hot_reloading_timer = 0.0f;

            {
                if (!compilation_process) {
                    STARTUPINFO startup_information = {
                        .cb = sizeof(startup_information),
                        .hStdError = GetStdHandle(STD_ERROR_HANDLE),
                        .hStdOutput = 0,

                        /* .hStdError = 0, */
                        /* .hStdOutput = 0, */
                        .dwFlags = STARTF_USESTDHANDLES
                    };
                    PROCESS_INFORMATION process_information = {};
                    if (CreateProcessA("C:/msys64/usr/bin/make.exe",
                                       "make client.dll", // doc states that this should be writable memory...
                                       NULL,
                                       NULL,
                                       true,
                                       0,
                                       NULL,
                                       ".",
                                       &startup_information,
                                       &process_information)) {
                        compilation_process = process_information.hProcess;
                        process_standard_input = startup_information.hStdInput;
                        process_standard_output = startup_information.hStdOutput;
                        process_standard_error_output = startup_information.hStdError;
                    } else {
                        /* printf("failed to start process???\n"); */
                    }
                } else {
                    switch (WaitForSingleObject(compilation_process, 0)) {
                        case WAIT_FAILED:
                        case WAIT_TIMEOUT:
                        case WAIT_ABANDONED:
                        {}
                        break;
                        case WAIT_OBJECT_0:
                        {
                            DWORD exit_code_status;
                            // Do a process checking API to make this a little less painful.
                            // also so the printing doesn't look stupid
                            if (GetExitCodeProcess(compilation_process, &exit_code_status)) {
                                if (exit_code_status != STILL_ACTIVE) {
                                    // make - 0 == success and built or no rebuild required
                                    // make - 1 == with -q and target needs rebuild
                                    // make - 2 == any errors
                                    CloseHandle(compilation_process);
                                    compilation_process = NULL;
                                    if (last_known_process_exit_code != exit_code_status) {
                                        switch (exit_code_status) {
                                            case 0:
                                            {
                                                /* printf("successful rebuild\n"); */
                                            }
                                            break;
                                            case 1:
                                            {
                                                printf("requires rebuild.\n");
                                            }
                                            break;
                                            case 2:
                                            {
                                                SetConsoleCursorPosition(GetStdHandle(STD_ERROR_HANDLE), (COORD){0, 0});
                                            } 
                                            break;
                                        }
                                        last_known_process_exit_code = exit_code_status;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (hot_reloading_timer <= 0.0f && !compilation_process) {
                update_win32_file_information("client.dll", &current_stat_information);
                if (win32_stat_timestamp_difference(&current_stat_information, &last_stat_information)) {
                    last_stat_information = current_stat_information;
                    printf("detected reload.\n");
                    if (client_function_pointers.on_unload) {
                        client_function_pointers.on_unload(&client_state);
                    }
                    
                    client_state_function_pointers_unload(&client_function_pointers);
                    if (last_known_process_exit_code == 0 || !win32_file_exists("hotloaded_client.dll")) {
                        CopyFile("client.dll", "hotloaded_client.dll", false);
                    }
                    client_state_function_pointers_update_from(&client_function_pointers, "hotloaded_client.dll");

                    if (client_function_pointers.on_reload) {
                        client_function_pointers.on_reload(&client_state);
                    }
                }
                hot_reloading_timer = SECONDS_BEFORE_HOT_RELOADING_CLIENT_DLL;
            }

            hot_reloading_timer -= delta_time;
        }
#endif

        
        {
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_WINDOWEVENT:
                    {
                        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                            window_resolution_w = event.window.data1;
                            window_resolution_h = event.window.data2;
                            if (client_function_pointers.on_resolution_change) {
                                client_function_pointers.on_resolution_change(&client_state, window_resolution_w, window_resolution_h);
                            }
                        }
                    }
                    break;
                    case SDL_KEYDOWN:
                    {
                        if (client_function_pointers.on_keydown) {
                            client_function_pointers.on_keydown(&client_state, event.key.keysym.sym);
                        }
                    }
                    break;
                    case SDL_KEYUP:
                    {
                        if (client_function_pointers.on_keyup) {
                            client_function_pointers.on_keyup(&client_state, event.key.keysym.sym);
                        }
                    }
                    break;
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    {
                        if (client_function_pointers.on_mouse_event) {
                            client_function_pointers.on_mouse_event(&client_state, event.motion.x, event.motion.y, event.button.button);
                        }
                    }
                    break;
                    case SDL_QUIT:
                    {
                        if (client_function_pointers.on_quit) {
                            client_function_pointers.on_quit(&client_state);
                        }
                        running = false;
                    }
                    break;
                    default: break;
                }
            }
        }

        if (client_function_pointers.on_update) {
            client_function_pointers.on_update(&client_state, delta_time);
        }

        if (client_function_pointers.on_draw) {
            client_function_pointers.on_draw(&client_state, renderer);
        }


        frame_end_time = SDL_GetTicks();
        delta_time = (frame_end_time - frame_start_time) / 1000.0;
    }

    {
        CloseHandle(log_file_handle);

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        TTF_CloseFont(editor_default_font);
        TTF_Quit();
        SDL_Quit();
    }
    free(client_state.characters);
    return 0;
}
