all: main.o
	gcc -o sim main.o `sdl-config --libs` -lSDL_image

main.o: main.c
#	gcc -c main.c `sdl-config --cflags` -lSDL_image -pedantic -W -Wall
	gcc -c main.c `sdl-config --cflags` -lSDL_image