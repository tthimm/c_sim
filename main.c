#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 20
#define AIR 0
#define DIRT_BLOCK 1
#define WATER 2

int map_width = 0;
int map_height = 0;
int **tileset;

/* load map file and fill tiles array */
void load_map(void) {
	int i = 0;
	int j = 0;
	int k = 0;
	int c;
	int nRet;
	FILE *map;
	size_t *t = malloc(0);
	char **gptr = (char **)malloc(sizeof(char*));
	*gptr = NULL;

	/* open mapfile */
	map = fopen("media/maps/world.map", "r");
	if(NULL == map) {
		printf("couldn't load map file\n");
		exit(EXIT_FAILURE);
	}

	/* get map_height & map_width */
	while( (nRet=getline(gptr, t, map)) > 0) {
		map_height++;
		if(map_width == 0) {
			map_width = strlen(*gptr) - 1;
		}
	}
	free(gptr);
	free(t);

	/* allocate memory for tileset array */
	tileset = (int **)malloc(map_width * sizeof(int *));
	if(NULL == tileset) {
		printf("not enough free ram");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map_width; k++) {
		tileset[k] = (int *)malloc(map_height * sizeof(int));
		if(NULL == tileset[k]) {
			printf("not enough free ram for row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* set position to begin of stream */
	rewind(map);

	/* fill tileset array with values from mapfile */
	while( (c = fgetc(map)) != EOF ) {
		/* if newline */
		if(c == 10) {
			j++;
			i = 0;
		}
		else {
			switch(c) {
				case '#':
					c = DIRT_BLOCK;
					break;
				case '~':
					c = WATER;
					break;
				default:
					c = AIR;
					break;
			}
			tileset[i++][j] = c;
		}
	}
	fclose(map);
}

/* blocks are solid */
int solid(int x, int y) {
	int new_x = x / BLOCK_SIZE;
	int new_y = y / BLOCK_SIZE;
	if( (y < 0) | (x < 0) | (y >= map_height * BLOCK_SIZE) | (x >= map_width * BLOCK_SIZE) |
			((tileset[new_x][new_y] != AIR) & (tileset[new_x][new_y] != WATER)) ) {
		return 1;
	}
	else {
		return 0;
	}
}

void set_bg_color(SDL_Surface *scr) {
	SDL_Rect dst;
	dst.x = 0;
	dst.y = 0;
	dst.w = SCREEN_WIDTH;
	dst.h = SCREEN_HEIGHT;
	SDL_FillRect(scr, &dst, SDL_MapRGB(scr->format, 47, 136, 248));
}

void create_dirt_block(SDL_Surface *scr, int x, int y) {
	SDL_Rect dstt;
	dstt.x = x;
	dstt.y = y;
	dstt.w = BLOCK_SIZE;
	dstt.h = BLOCK_SIZE;
	SDL_FillRect(scr, &dstt, SDL_MapRGB(scr->format, 107, 73, 14));
	SDL_UpdateRect(scr, x, y, BLOCK_SIZE, BLOCK_SIZE);
}

int main(void) {
	int i, j, k;
	char block;
	SDL_Surface *screen;
	SDL_Event event;
	int done = 0;
	load_map();
/*	printf("%c solid? => %i\n", tiles[MAP_WIDTH-1][MAP_HEIGHT-1], solid(MAP_WIDTH-1, MAP_HEIGHT-1));
	printf("%c solid? => %i\n", tiles[0][0], solid(0, 0)); */


	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf("Can't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE);
	if(screen == NULL) {
		printf("Can't set video mode: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
/*	image = IMG_Load("media/images/dirt.png");
	if(image == NULL) {
		printf("Can't load dirt image: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}*/

	/* set bg color */
	set_bg_color(screen);
	/* srand(time(NULL)); */
	for(i = 0; i < map_width; i++) {
		for(j = 0; j < map_height; j++) {
			block = tileset[i][j];

			/*block = rand() % 2;*/
			/* print random generated numbers */
			/* printf("%i ", block); */
			if(block == 1) {
				create_dirt_block(screen, i*BLOCK_SIZE, j*BLOCK_SIZE);
			}
		}
	}

/*	SDL_BlitSurface(image, NULL, screen, NULL);
	SDL_FreeSurface(image); */
	SDL_UpdateRect(screen, 0, 0, 0, 0);
	while(!done) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				done = 1;
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
						done = 1;
						break;
				}
				break;
			}
		}
		SDL_Delay(1000/60);
	}
	for(i = 0; i < map_height; i++) {
		free(tileset[i]);
	}
	free(tileset);
	return 0;
}

