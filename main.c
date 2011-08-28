/* TODO: add inventory
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
#include <SDL/SDL_image.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define GRAVITY 0.8
#define PLAYER_SPEED 4
#define MAX_FALL_SPEED 20
#define SCROLL_SPEED PLAYER_SPEED
#define MAX_MASS 1.0
#define MAX_COMPRESS 0.02 //(MAX_MASS / 50)
#define MIN_MASS 0.0001
#define MAX_SPEED 1 /* max units of water moved out of one block to another, per timestep */
#define MIN_FLOW 0.01 // (MAX_MASS / 100)
#define water1_mass (MAX_MASS * 0.005)
#define water2_mass (MAX_MASS * 0.2)
#define water3_mass (MAX_MASS * 0.4)
#define water4_mass (MAX_MASS * 0.6)
#define water5_mass (MAX_MASS * 0.8)

/* tile index  -1   0     20     40    60     80      100     120     140     160*/
enum blocks { AIR, DIRT, GRASS, SAND, STONE, WATER1, WATER2, WATER3, WATER4, WATER5 };

struct Map {
	char *filename;
	int blocksize, w, h, min_x, min_y, max_x, max_y;
	int **tiles;
	float **mass;
	float **new_mass;
} map = {"media/maps/world.map", 20, 0, 0};

struct Bg_color {
	int r;
	int g;
	int b;
} bg = {47, 136, 248};

struct Player {
	int x, y, w, h, on_ground, selected;
	float vx, vy;
} player = { 0, 0, 26, 38, 1, 0, 0, 0};

typedef struct Input {
	int left, right, jump, mleft, mright;
} Input;

SDL_Surface *temp, *tileset, *player_image, *cursor;
Input input;
int mouse_x = 0;
int mouse_y = 0;

/* load map file and fill tiles array */
void load_map(char *name) {
	int c, i, j, k;
	i = j = k = 0;
	int nRet;
	FILE *mmap;
	size_t *t = malloc(0);
	char **gptr = (char **)malloc(sizeof(char*));
	*gptr = NULL;

	/* open mapfile */
	mmap = fopen(name, "r");
	if(NULL == mmap) {
		printf("couldn't load map file\n");
		exit(EXIT_FAILURE);
	}

	/* get map_height & map_width */
	while( (nRet=getline(gptr, t, mmap)) > 0) {
		map.h++;
		if(map.w == 0) {
			map.w = strlen(*gptr) - 1;
		}
	}
	free(gptr);
	free(t);

	/* allocate memory for tileset array */
	map.tiles = (int **)malloc(map.w * sizeof(int *));
	if(NULL == map.tiles) {
		printf("not enough free ram");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map.w; k++) {
		map.tiles[k] = (int *)malloc(map.h * sizeof(int));
		if(NULL == map.tiles[k]) {
			printf("not enough free ram for row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* allocate memory for mass array */
	map.mass = (float **)malloc(map.w * sizeof(float *));
	if(NULL == map.mass) {
		printf("not enough free ram (mass)");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map.w; k++) {
		map.mass[k] = (float *)malloc(map.h * sizeof(float));
		if(NULL == map.mass[k]) {
			printf("not enough free ram for (mass)row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* allocate memory for new_mass array */
	map.new_mass = (float **)malloc(map.w * sizeof(float *));
	if(NULL == map.new_mass) {
		printf("not enough free ram (new_mass)");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map.w; k++) {
		map.new_mass[k] = (float *)malloc(map.h * sizeof(float));
		if(NULL == map.new_mass[k]) {
			printf("not enough free ram for (new_mass)row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* reset position to begin of stream */
	rewind(mmap);

	/* fill tiles/mass/new_mass array with values from mapfile */
	while( (c = fgetc(mmap)) != EOF ) {
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
				case 'p':
					player.x = i * map.blocksize;
					player.y = j * map.blocksize;
					c = AIR;
					break;
				default:	c = AIR;		break;
			}
			map.tiles[i][j] = c;
			map.mass[i][j] = (c == WATER5 ? 1.0 : 0.0);
			map.new_mass[i][j] = (c == WATER5 ? 1.0 : 0.0);
			i++;
		}
	}
	fclose(mmap);

	/* set min and max scroll position of the map */
	map.min_x = map.min_y = 0;
	map.max_x = map.w * map.blocksize;
	map.max_y = map.h * map.blocksize;
}

/* blocks are solid, water and air aren't */
int solid(int x, int y) {
	int dx = x/map.blocksize;
	int dy = y/map.blocksize;
	if((dx < map.w) && (dy < map.h)) { /* fix for trying to access index out of bounds > */
		int tile = map.tiles[dx][dy];
		if( (y < 0) || (x < 0) || (dy >= map.h) || (dx >= map.w) ||
			((tile != AIR) && (tile < WATER1)) || ((tile != AIR) && (tile > WATER5))) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 1;
	}
}

int player_speed(struct Player *p) {
	int dx = p->x/map.blocksize;
	int dy = (p->y + p->h) / map.blocksize - 1;
	if((map.tiles[dx][dy] == WATER1) || (map.tiles[dx][dy] == WATER2) || (map.tiles[dx][dy] == WATER3) ||
			(map.tiles[dx][dy] == WATER4) || (map.tiles[dx][dy] == WATER5)) {
		return PLAYER_SPEED/2;
	}
	else {
		return PLAYER_SPEED;
	}
}

int player_fall_speed(struct Player *p) {
	int dx = p->x/map.blocksize;
	int dy = (p->y + p->h) / map.blocksize - 1;
	if((map.tiles[dx][dy] == WATER1) || (map.tiles[dx][dy] == WATER2) || (map.tiles[dx][dy] == WATER3) ||
			(map.tiles[dx][dy] == WATER4) || (map.tiles[dx][dy] == WATER5)) {
		return MAX_FALL_SPEED/5;
	}
	else {
		return MAX_FALL_SPEED;
	}
}

int not_solid_above(struct Player *p) {

	if((!solid(p->x, p->y - 1)) &&
			(!solid(p->x + (p->w / 2), p->y - 1)) &&
			(!solid(p->x + (p->w - 1), p->y - 1))) {
		return 1;
	}
	else {
		return 0;
	}
}

void on_ground(struct Player *p) {
	if(solid(p->x, p->y + p->h) ||
			solid(p->x + (p->w / 2), p->y + p->h) ||
			solid(p->x + p->w - 1, p->y + p->h)) {
		p->on_ground = 1;
	}
	else {
		p->on_ground = 0;
	}
}

/* draw background color */
void draw_background(SDL_Surface *scr) {
	SDL_Rect dst;
	dst.x = 0;
	dst.y = 0;
	dst.w = SCREEN_WIDTH;
	dst.h = SCREEN_HEIGHT;
	SDL_FillRect(scr, &dst, SDL_MapRGB(scr->format, bg.r, bg.g, bg.b));
}

void load_tileset(void) {
	temp = IMG_Load("media/images/tileset20_full.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY, SDL_MapRGB(temp->format, 255, 0, 0));
	/*SDL_SetAlpha(tileset, SDL_SRCALPHA, 170);*/
	tileset = SDL_DisplayFormat(temp);
	if(NULL == tileset) {
		printf("Can't load tileset: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

/* draw the tile */
void draw_tile(SDL_Surface *screen, int x, int y, int tile_x) {
	SDL_Rect src, dst;
	src.x = tile_x;
	src.y = 0;
	src.w = map.blocksize;
	src.h = map.blocksize;

	dst.x = x;
	dst.y = y;
	dst.w = map.blocksize;
	dst.h = map.blocksize;
	SDL_BlitSurface(tileset, &src, screen, &dst);
}

/* iterate through tiles array and draw tiles */
void draw_all_tiles(SDL_Surface *screen) {
	int x, y, tileset_index;
	int x1, x2, y1, y2, map_x, map_y;

	map_x = map.min_x / map.blocksize;
	x1 = (map.min_x % map.blocksize) * -1;
	x2 = x1 + SCREEN_WIDTH + (x1 == 0 ? 0 : map.blocksize);

	map_y = map.min_y / map.blocksize;
	y1 = (map.min_y % map.blocksize) * -1;
	y2 = y1 + SCREEN_HEIGHT + (y1 == 0 ? 0 : map.blocksize);

	for(y = y1; y < y2; y += map.blocksize) {
		map_x = map.min_x / map.blocksize;
		for(x = x1; x < x2; x += map.blocksize) {
			tileset_index = map.tiles[map_x][map_y] - 1;
			draw_tile(screen, x, y, tileset_index * map.blocksize);
			map_x++;
		}
		map_y++;
	}
}

void load_player_image(void) {
	temp = IMG_Load("media/images/player.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY | SDL_RLEACCEL, (Uint16) SDL_MapRGB(temp->format, 255, 0, 0));
	player_image = SDL_DisplayFormat(temp);
	if(NULL == tileset) {
		printf("Can't load player: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

void check_against_map(struct Player *p) {
	int i;

	/* move down / fall */
	if(p->vy > 0) {
		for(i = 0; i < p->vy; i++) {
			on_ground(p);
			if(!p->on_ground) {
				p->y += 1;
			}
			else {
				break;
			}
		}
	}

	/* jump */
	if(p->on_ground) {
		if(p->vy < 0) {
			for(i = 0; i > p->vy; i--) {
				if(not_solid_above(p)) {
					p->y -= 1;
				}
				else {
					/* reset upward movement to prevent jumping if player hits above block and then moves left/right */
					p->vy = 0;
				}
			}
		}
	}

	/* move right */
	if(p->vx > 0) {
		for(i = 0; i < p->vx; i++) {
			if((!solid(p->x + p->w, p->y)) && (!solid(p->x + p->w, p->y + p->h/2 - 1)) && (!solid(p->x + p->w, p->y + p->h - 1))) {
				p->x += 1;
			}
		}
	}

	/* move left */
	if(p->vx < 0) {
		for(i = 0; i > p->vx; i--) {
			if((!solid(p->x - 1, p->y)) && !solid(p->x - 1, p->y + p->h/2 - 1) && !solid(p->x - 1, p->y + p->h - 1)) {
				p->x -= 1;
			}
		}
	}
}

void center_camera(struct Player *p) {
	map.min_x = p->x - (SCREEN_WIDTH / 2);
	if(map.min_x < 0) {
		map.min_x = 0;
	}

	if(map.min_x + SCREEN_WIDTH >= map.max_x) {
		map.min_x = map.max_x - SCREEN_WIDTH;
	}

	map.min_y = p->y - (SCREEN_HEIGHT / 2);
	if(map.min_y < 0) {
		map.min_y = 0;
	}

	if(map.min_y + SCREEN_HEIGHT >= map.max_y) {
		map.min_y = map.max_y - SCREEN_HEIGHT;
	}
}

void move_player(struct Player *p) {
	int fall_speed = player_fall_speed(p);
	int speed = player_speed(p);
	p->vx = 0;
	p->vy += GRAVITY;

	if(p->vy >= fall_speed) {
		p->vy = fall_speed;
	}

	if(input.left == 1) {
		p->vx -= speed;
	}

	if(input.right == 1) {
		p->vx += speed;
	}

	if(input.jump == 1) {
		if(p->on_ground) {
			p->vy = -9;
		}
	}

	input.jump = 0;
	check_against_map(p);
	center_camera(p);
}

void draw_player(SDL_Surface *screen, struct Player *p) {
	SDL_Rect src, dst;
	src.x = 0;
	src.y = 0;
	src.w = p->w;
	src.h = p->h;

	dst.x = p->x - map.min_x;
	dst.y = p->y - map.min_y;
	dst.w = p->w;
	dst.h = p->h;
	SDL_BlitSurface(player_image, &src, screen, &dst);
}

/* destroy block */
void destroy_block(SDL_Surface *scr, int x, int y) {
	int dx = (x + map.min_x)/map.blocksize;
	int dy = (y + map.min_y)/map.blocksize;
	if((dx < map.w) && (dy < map.h)) {
		int tile = map.tiles[dx][dy];

		/* if tile is not air and not water */
		if((tile != AIR)) {/* & ((tile < WATER1) | (tile > WATER5))) {*/
			map.tiles[dx][dy] = AIR;
			map.new_mass[dx][dy] = 0;
		}
	}
}

/* place block if there is enough room */
void place_block(int x, int y, struct Player *p) {
	int cam_x = x + map.min_x;
	int cam_y = y + map.min_y;
	int dx = cam_x/map.blocksize;
	int dy = cam_y/map.blocksize;
	int player_x = (p->x + map.min_x)/map.blocksize;
	int player_y = (p->y + map.min_y)/map.blocksize;

	if((dx < map.w) && (dy < map.h) && not_player_position(dx, dy, p) ) {
		/* not solid at new block position, dirt selected &
		horizontal or vertical adjacent block exists (never place floating blocks) */
		if(!solid(cam_x, cam_y) && (solid(cam_x + map.blocksize, cam_y) ||
				solid(cam_x - map.blocksize, cam_y) ||
				solid(cam_x, cam_y + map.blocksize) ||
				solid(cam_x, cam_y - map.blocksize))) {
			if(p->selected == 0) {
				map.tiles[dx][dy] = DIRT;
				//map.mass[dx][dy] = 0;
				map.new_mass[dx][dy] = 0;
			}
		}
		/* not solid at new block position and water selected */
		if(!solid(cam_x, cam_y) && (p->selected == 1)) {
			map.tiles[dx][dy] = WATER5;
			map.new_mass[dx][dy] = MAX_MASS;
		}
	}
}

void place_and_destroy_blocks(SDL_Surface *scr, int x, int y, struct Player *p) {
	// if mouse button is pressed and player moves cursor out of the screen, x and y could be negative
	if((x > 0) && (y > 0)) {
		if(input.mleft) {
			destroy_block(scr, x, y);
		}
		if(input.mright) {
			place_block(x, y, p);
		}
	}
}

/* returns 1 if click position is not player position */
int not_player_position(int x, int y, struct Player *p) {
	int x1, x2, x3, y1, y2, y3;
	x1 = (p->x) / map.blocksize;
	x2 = ((p->x) + p->w / 2) / map.blocksize;
	x3 = ((p->x) + p->w - 1) / map.blocksize;
	y1 = (p->y) / map.blocksize;
	y2 = ((p->y) + p->h / 2) / map.blocksize;
	y3 = ((p->y) + p->h - 1) / map.blocksize;
	if( !((x == x1) && (y == y1)) &&
			!((x == x2) && (y == y2)) &&
			!((x == x3) && (y == y3)) ) {
		return 1;
	}
	else {
		return 0;
	}
}

void load_cursor(void) {
	temp = IMG_Load("media/images/cursor_small.png");
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
	src.w = map.blocksize;
	src.h = map.blocksize;

	dst.x = x;//(x/map.blocksize)*map.blocksize;
	dst.y = y;//(y/map.blocksize)*map.blocksize;
	dst.w = map.blocksize;
	dst.h = map.blocksize;
	SDL_BlitSurface(cursor, &src, screen, &dst);
}

void draw(SDL_Surface *screen, int mouse_x, int mouse_y, struct Player *p) {
	draw_background(screen);
	draw_all_tiles(screen);
	draw_player(screen, p);
	draw_cursor(screen, mouse_x, mouse_y);

	SDL_Flip(screen);
	SDL_Delay(1);
}

void delay(unsigned int frame_limit) {
	unsigned int ticks = SDL_GetTicks();

	if (frame_limit < ticks) {
		return;
	}

	if (frame_limit > ticks + (1000/60)) {
		SDL_Delay((1000/60));
	}
	else {
		SDL_Delay(frame_limit - ticks);
	}
}

/* determines the smallest value of two numbers */
float min(float i, float j) {
	float smallest;
	if(i <= j) {
		smallest = i;
	}
	else {
		smallest = j;
	}
	return smallest;
}

/* constrains a value to not exceed a maximum and minimum value */
float constrain(float value, float min, float max) {
	if(value <= min) {
		return min;
	}
	else if(value > max) {
		return max;
	}
	else {
		return value;
	}
}

/* returns the amount of water that should be in the bottom cell */
float get_stable_state_below(float total_mass) {
	if(total_mass <= MAX_MASS) {
		return MAX_MASS;
	}
	else if(total_mass < (2 * MAX_MASS) + MAX_COMPRESS ) {
		return ((MAX_MASS * MAX_MASS) + (total_mass * MAX_COMPRESS)) / (MAX_MASS + MAX_COMPRESS);
	}
	else {
		return (total_mass + MAX_COMPRESS) / 2;
	}
}

void simulate_compression(void) {
	float flow = 0;
	float remaining_mass;
	int x, y;

	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			/* skip non water blocks */
			if(map.tiles[x][y] != AIR && map.tiles[x][y] < WATER1) {
				continue;
			}

			flow = 0;
			remaining_mass = map.mass[x][y];
			if(remaining_mass <= 0) {
				continue;
			}

			/* the block below this one */
			if(y+1 < map.h) {
				if((map.tiles[x][y+1] == AIR) || (map.tiles[x][y+1] >= WATER1)) {
					flow = get_stable_state_below(remaining_mass + map.mass[x][y+1]) - map.mass[x][y+1];
					if(flow > MIN_FLOW) {
						flow *= 0.5; /* leads to smoother flow */
					}
					flow = constrain(flow, 0, min(MAX_SPEED, remaining_mass));

					map.new_mass[x][y] -= flow;
					map.new_mass[x][y+1] += flow;
					remaining_mass -= flow;
				}
			}

			if(remaining_mass <= 0) {
				continue;
			}

			/* block left of this one */
			if(x-1 >= 0) {
				if((map.tiles[x-1][y] == AIR) || (map.tiles[x-1][y] >= WATER1)) {
					/* equalize the amount of water in this block and it's neighbour */
					flow = (map.mass[x][y] - map.mass[x-1][y]) / 4;
					if(flow > MIN_FLOW) {
						flow *= 0.5;
					}
					flow = constrain(flow, 0, remaining_mass);

					map.new_mass[x][y] -= flow;
					map.new_mass[x-1][y] += flow;
					remaining_mass -= flow;
				}
			}

			if(remaining_mass <= 0) {
				continue;
			}

			/* block right of this one */
			if(x + 1 < map.w) {
				if((map.tiles[x+1][y] == AIR) || (map.tiles[x+1][y] >= WATER1)) {
					/* equalize the amount of water in this block and it's neighbour */
					flow = (map.mass[x][y] - map.mass[x+1][y]) / 4;
					if(flow > MIN_FLOW) {
						flow *= 0.5;
					}
					flow = constrain(flow, 0, remaining_mass);

					map.new_mass[x][y] -= flow;
					map.new_mass[x+1][y] += flow;
					remaining_mass -= flow;
				}
			}

			if(remaining_mass <= 0) {
				continue;
			}

			/* up. only compressed water flows upwards */
			/*if(y-1 >= 0) {
				if((map.tiles[x][y-1] == AIR) || (map.tiles[x][y-1] >= WATER1)) {
					flow = remaining_mass - get_stable_state_below(remaining_mass + map.mass[x][y-1]);
					if(flow > MIN_FLOW) {
						flow *= 0.5;
					}
					flow = constrain(flow, 0, min(MAX_SPEED, remaining_mass));

					map.new_mass[x][y] -= flow;
					map.new_mass[x][y-1] += flow;
					remaining_mass -= flow;
				}
			}*/
		}
	}
	/* copy the new mass values to the mass array */
	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			map.mass[x][y] = map.new_mass[x][y];
		}
	}

	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			/* skip ground blocks */
			if(map.tiles[x][y] != AIR && map.tiles[x][y] < WATER1) {
				continue;
			}
			/* Flag/unflag water blocks */
			if ((map.mass[x][y] >= MIN_MASS) && (map.mass[x][y] < water2_mass)) {
				map.tiles[x][y] = WATER1;
			}
			else if ((map.mass[x][y] >= water2_mass) && (map.mass[x][y] < water3_mass)) {
				map.tiles[x][y] = WATER2;
			}
			else if ((map.mass[x][y] >= water3_mass) && (map.mass[x][y] < water4_mass)) {
				map.tiles[x][y] = WATER3;
			}
			else if ((map.mass[x][y] >= water4_mass) && (map.mass[x][y] < water5_mass)) {
				map.tiles[x][y] = WATER4;
			}
			else if (map.mass[x][y] >= water5_mass) {
				map.tiles[x][y] = WATER5;
			}
			else {
				map.tiles[x][y] = AIR;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	SDL_Surface *screen;
	SDL_Event event;
	int i, done = 0;
	unsigned int frame_limit = SDL_GetTicks() + (1000/60);

	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf("Can't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	//screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_FULLSCREEN | SDL_DOUBLEBUF);
	if(NULL == screen) {
		printf("Can't set video mode: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	/* disable cursor */
	SDL_ShowCursor(SDL_DISABLE);

	/* load new cursor */
	load_cursor();

	/* load the map and fill tiles array */
	load_map(map.filename);

	/* load tileset */
	load_tileset();

	/* load player */
	load_player_image();

	/* game loop */
	while(!done) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = 1;
					break;
				case SDL_MOUSEMOTION:
					mouse_x = event.motion.x;
					mouse_y = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(event.button.button) {
						case 1:
							//destroy_block(screen, event.button.x, event.button.y);
							input.mleft = 1;
							break;
						case 3:
							//place_block(event.button.x, event.button.y, &player);
							input.mright = 1;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(event.button.button) {
						/* removed because if player falls down, it deletes always the block below the player */
						case 1:
							input.mleft = 0;
							break;
						case 3:
							input.mright = 0;
							break;
						default:
							break;
					}
					break;

				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_ESCAPE:
							done = 1;
							break;
						case SDLK_a:
							input.left = 1;
							break;
						case SDLK_d:
							input.right = 1;
							break;
						case SDLK_SPACE:
							input.jump = 1;
							break;
						case SDLK_1:
							player.selected = 0;
							break;
						case SDLK_2:
							player.selected = 1;
							break;
						default:
							break;
					}
					break;
				case SDL_KEYUP:
					switch(event.key.keysym.sym) {
						case SDLK_a:
							input.left = 0;
							break;
						case SDLK_d:
							input.right = 0;
							break;
						default:
							break;
					}
					break;
			}
		}
		move_player(&player); // and camera
		simulate_compression();
		place_and_destroy_blocks(screen, event.button.x, event.button.y, &player);
		//input.mleft = 0; // uncomment for click once to delete one block
		draw(screen, mouse_x, mouse_y, &player);

		delay(frame_limit);
		frame_limit = SDL_GetTicks() + (1000/60);
	}

	/* free tiles/mass/new_mass array in reverse order */
	for(i = 0; i < map.h; i++) {
		free(map.tiles[i]);
		free(map.mass[i]);
		free(map.new_mass[i]);
	}
	free(map.tiles);
	free(map.mass);
	free(map.new_mass);

	SDL_FreeSurface(tileset);
	SDL_FreeSurface(player_image);
	SDL_FreeSurface(cursor);
	return 0;
}
