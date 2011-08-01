all: main.o
	gcc -o sim main.o `sdl-config --libs`

main.o: main.c
	gcc -c main.c `sdl-config --cflags` -lSDL_image