/* TODO: camera and translate blocks (see libgosu)
 * TODO: player controls
 * TODO: flowing water (see http://w-shadow.com/blog/2009/09/01/simple-fluid-simulation/)
 * TODO: add inventory
 * TODO: add hud
 * TODO: add minerals
 * TODO: add liquids (oil, lava)
 * TODO: add tools
*/
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "SDL/SDL_image.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 20
//             -1   0     20     40    60     80      100     120     140     160
enum blocks { AIR, DIRT, GRASS, SAND, STONE, WATER1, WATER2, WATER3, WATER4, WATER5 };

int map_width = 0;
int map_height = 0;
int **tiles;
SDL_Surface *temp, *tileset, *player_sf, *cursor;
int mouse_x = 0;
int mouse_y = 0;

struct bg_color {
	int r;
	int g;
	int b;
} bg = {47, 136, 248};

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
	tiles = (int **)malloc(map_width * sizeof(int *));
	if(NULL == tiles) {
		printf("not enough free ram");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map_width; k++) {
		tiles[k] = (int *)malloc(map_height * sizeof(int));
		if(NULL == tiles[k]) {
			printf("not enough free ram for row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* set position to begin of stream */
	rewind(map);

	/* fill tiles array with values from mapfile */
	while( (c = fgetc(map)) != EOF ) {
		/* if newline */
		if(c == 10) {
			j++;
			i = 0;
		}
		else {
			switch(c) {
				case '#':	c = DIRT;		break;
				case 'G':	c = GRASS;		break;
				case 'S':	c = SAND;		break;
				case 'R':	c = STONE;		break;
				case '1':	c = WATER1;		break;
				case '2':	c = WATER2;		break;
				case '3':	c = WATER3;		break;
				case '4':	c = WATER4;		break;
				case '5':	c = WATER5;		break;
				default:	c = AIR;		break;
			}
			tiles[i++][j] = c;
		}
	}
	fclose(map);
}

/* blocks are solid, water and air aren't */
int solid(int x, int y) {
	int dx = x/BLOCK_SIZE;
	int dy = y/BLOCK_SIZE;
	int tile = tiles[dx][dy];
	if( (y < 0) | (x < 0) | (y >= map_height * BLOCK_SIZE) | (x >= map_width * BLOCK_SIZE) |
			((tile != AIR) & (tile < WATER1)) | ((tile != AIR) & (tile > WATER5))) {
		return 1;
	}
	else {
		return 0;
	}
}

/* set background color */
void set_bg_color(SDL_Surface *scr) {
	SDL_Rect dst;
	dst.x = 0;
	dst.y = 0;
	dst.w = SCREEN_WIDTH;
	dst.h = SCREEN_HEIGHT;
	SDL_FillRect(scr, &dst, SDL_MapRGB(scr->format, bg.r, bg.g, bg.b));
}

void load_tileset(void) {
	temp = IMG_Load("media/images/tileset20.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY, SDL_MapRGB(temp->format, 255, 0, 0));
	/*SDL_SetAlpha(tileset, SDL_SRCALPHA, 170);*/
	tileset = SDL_DisplayFormat(temp);
	if(NULL == tileset) {
		printf("Can't load tileset: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

/* draw background for deleted and transparent tiles */
void draw_dirty_rect(SDL_Surface *screen, int x, int y) {
	SDL_Rect dirty_rect;
	dirty_rect.x = (x/BLOCK_SIZE)*BLOCK_SIZE;
	dirty_rect.y = (y/BLOCK_SIZE)*BLOCK_SIZE;
	dirty_rect.w = BLOCK_SIZE;
	dirty_rect.h = BLOCK_SIZE;
	SDL_FillRect(screen, &dirty_rect, SDL_MapRGB(screen->format, bg.r, bg.g, bg.b));
}

/* draw the tile */
void really_draw_tile(SDL_Surface *screen, int x, int y, int tile_x) {
	SDL_Rect src, dst;
	src.x = tile_x;
	src.y = 0;
	src.w = BLOCK_SIZE;
	src.h = BLOCK_SIZE;

	dst.x = x;
	dst.y = y;
	dst.w = BLOCK_SIZE;
	dst.h = BLOCK_SIZE;
	SDL_BlitSurface(tileset, &src, screen, &dst);
}

/* sometimes we can draw directly and sometimes we must draw the dirty rect first */
void draw_tile(SDL_Surface *screen, int x, int y, int tile_x) {
	// AIR tile
	if(tile_x == -20) {
		draw_dirty_rect(screen, x, y);
	}
	// transparent WATER tiles
	else if((tile_x >= 80) & (tile_x <= 140)) {
		draw_dirty_rect(screen, x, y);
		really_draw_tile(screen, x, y, tile_x);
	}
	// any other tiles
	else {
		really_draw_tile(screen, x, y, tile_x);
	}
}

/* iterate through tiles array and draw tiles */
void draw_all_tiles(SDL_Surface *screen) {
	int i, j, tileset_index;
	for(i = 0; i < map_width; i++) {
		for(j = 0; j < map_height; j++) {
			tileset_index = tiles[i][j] - 1;
			draw_tile(screen, i*BLOCK_SIZE, j*BLOCK_SIZE, tileset_index * BLOCK_SIZE);
		}
	}
}

struct player {
	int x;
	int y;
	int vx;
	int vy;
} p = { 0, 0, 0, 0};

void load_player_image(void) {
	temp = IMG_Load("media/images/player.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY | SDL_RLEACCEL, (Uint16) SDL_MapRGB(temp->format, 255, 0, 0));
	player_sf = SDL_DisplayFormat(temp);
	if(NULL == tileset) {
		printf("Can't load player: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

void draw_player(SDL_Surface *screen, struct player *p) {
	SDL_Rect src, dst;
	src.x = 0;
	src.y = 0;
	src.w = 25;
	src.h = 45;

	dst.x = p->x;
	dst.y = p->y;
	dst.w = 25;
	dst.h = 45;
	SDL_BlitSurface(player_sf, &src, screen, &dst);
}

/* destroy block */
void destroy_block(SDL_Surface *scr, int x, int y) {
	int dx = x/BLOCK_SIZE;
	int dy = y/BLOCK_SIZE;
	int tile = tiles[dx][dy];

	/* if tile is not air and not water */
	if((tile != AIR) & ((tile < WATER1) | (tile > WATER5))) {
		tiles[dx][dy] = AIR;
		draw_dirty_rect(scr, x, y);
	}
}

/* place block if there is enough room */
void place_block(int x, int y) {
	int dx = x/BLOCK_SIZE;
	int dy = y/BLOCK_SIZE;
	int tile = tiles[dx][dy];

	/* if tile is air or any water block */
	if((tile == AIR) | ((tile >= WATER1) & (tile <= WATER5))) {
		tiles[dx][dy] = DIRT;
	}

}

void load_cursor(void) {
	temp = IMG_Load("media/images/cursor20.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY, SDL_MapRGB(temp->format, 255, 0, 0));
	SDL_SetAlpha(temp, SDL_SRCALPHA, 100);
	cursor = SDL_DisplayFormat(temp);
	if(NULL == cursor) {
		printf("Can't load cursor: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

void draw_cursor(SDL_Surface *screen, int x, int y) {
	SDL_Rect src, dst;
	src.x = 0;
	src.y = 0;
	src.w = BLOCK_SIZE;
	src.h = BLOCK_SIZE;

	dst.x = (x/BLOCK_SIZE)*BLOCK_SIZE;
	dst.y = (y/BLOCK_SIZE)*BLOCK_SIZE;
	dst.w = BLOCK_SIZE;
	dst.h = BLOCK_SIZE;
	SDL_BlitSurface(cursor, &src, screen, &dst);
}

int main(void) {
	SDL_Surface *screen;
	SDL_Event event;
	int i, done = 0;

	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf("Can't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if(NULL == screen) {
		printf("Can't set video mode: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	/* disable cursor */
	SDL_ShowCursor(SDL_DISABLE);

	/* load new cursor */
	load_cursor();

	/* load the map and fill tiles array */
	load_map();

	/* set bg color */
	set_bg_color(screen);

	/* load tileset */
	load_tileset();

	/* load player */
	load_player_image();

	/* game loop */
	while(!done) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_MOUSEMOTION:
					mouse_x = event.motion.x;
					mouse_y = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(event.button.button) {
						case 1:
							destroy_block(screen, event.button.x, event.button.y);
							break;
						case 3:
							place_block(event.button.x, event.button.y);
							break;
					}
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

		/* draw the tiles */
		draw_all_tiles(screen);
		draw_player(screen, &p);
		draw_cursor(screen, mouse_x, mouse_y);

		SDL_Flip(screen);
		SDL_Delay(1000/60);
	}

	/* free tiles array in reverse order */
	for(i = 0; i < map_height; i++) {
		free(tiles[i]);
	}
	free(tiles);

	SDL_FreeSurface(tileset);
	SDL_FreeSurface(player_sf);
	SDL_FreeSurface(cursor);
	return 0;
}
