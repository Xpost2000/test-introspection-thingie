CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -ggdb3 -Wno-unused-parameter -Wno-unused-function -g

.PHONY: all
all: host.exe client.dll
host.exe: basic_rtti.c client_state.h game_stuff.h main.c
	$(CC) $(CFLAGS) main.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -o host.exe
client.dll: client_dll.c basic_rtti.c game_stuff.h
	$(CC) $(CFLAGS) client_dll.c -c -lSDL2 -lSDL2_ttf -o client_dll.o -w
	$(CC) $(CFLAGS) client_dll.o -lSDL2 -lSDL2_ttf -o client.dll -s -shared -w

