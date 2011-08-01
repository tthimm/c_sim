#include <stdlib.h>
#include <time.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

#define MAP_WIDTH 16
#define MAP_HEIGHT 12
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 50

int tiles[MAP_WIDTH][MAP_HEIGHT];

// load map file and fill tiles array
void load_map(void) {
	int i = 0;
	int j = 0;
	int  c;
	FILE *map;

	map = fopen("media/maps/world.map", "r");
	if(NULL == map) {
		printf("couldn't load map file\n");
		exit(EXIT_FAILURE);
	}
	else {
		while( (c = fgetc(map)) != EOF ) {
			if(c == 10) {
				j++;
				i = 0;
			}
			else {
				tiles[i++][j] = c;
			}
		}
		fclose(map);
	}
}

// blocks are solid
int solid(int x, int y) {

	if(tiles[x/50][y/50] == '0') {
		return 0;
	}
	else {
		return 1;
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
	int coord, i, j;
	char block;
	load_map();
//	printf("%c solid? => %i\n", tiles[MAP_WIDTH-1][MAP_HEIGHT-1], solid(MAP_WIDTH-1, MAP_HEIGHT-1));
//	printf("%c solid? => %i\n", tiles[0][0], solid(0, 0));
	SDL_Surface *screen, *image;
	SDL_Event event;
	int done = 0;
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
//	image = IMG_Load("media/images/dirt.png");
//	if(image == NULL) {
//		printf("Can't load dirt image: %s", SDL_GetError());
//		exit(EXIT_FAILURE);
//	}

	// set bg color
	set_bg_color(screen);
	// remove me for real map loading
	//srand(time(NULL));
	for(i = 0; i < MAP_WIDTH; i++) {
		for(j = 0; j < MAP_HEIGHT; j++) {
			block = tiles[i][j];
			// remove me for real map loading
			//block = rand() % 2;
			// print random generated numbers
			//printf("%i ", block);
			if(block == 1 | block == '1') {
				create_dirt_block(screen, i*BLOCK_SIZE, j*BLOCK_SIZE);
			}
		}
	}

//	SDL_BlitSurface(image, NULL, screen, NULL);
//	SDL_FreeSurface(image);
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
			}
		}
		SDL_Delay(1000/60);
	}
	return 0;
}

