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
#include <SDL/SDL_ttf.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define GRAVITY 0.8
#define PLAYER_SPEED 4
#define MAX_FALL_SPEED 20
#define SCROLL_SPEED PLAYER_SPEED
#define FLIMIT (1000/60)
#define SAVE_EVENT (SDL_USEREVENT + 1)
#define FLOW_EVENT (SDL_USEREVENT + 2)

const unsigned char MAX_MASS = 5;
const unsigned char MIN_MASS = 1;

/* tile index  -1   0     20     40    60     80      100     120     140     160    180 */
enum cells { AIR, DIRT, GRASS, SAND, ROCK, WATER1, WATER2, WATER3, WATER4, WATER5, OIL };
enum calc { FALSE, TRUE };
enum direction { NONE, LEFT, RIGHT };

struct Cell {
	unsigned cell_type:4;	// 0/16
	unsigned direction:2;	// 0/4
	unsigned mass:4;		// 0/16
	unsigned calc:1;		// 0/1
} cell[262144];

struct Map {
	char *filename;
	int cellsize, w, h, min_x, min_y, max_x, max_y;
	int **grid;
} map = {"media/maps/world.map", 20, 0, 0};

struct Bg_color {
	int r;
	int g;
	int b;
} bg = {47, 136, 248};

struct Player {
	unsigned int x, y, w, h, on_ground, selected;
	float vx, vy;
} player = { 0, 0, 26, 38, 1, WATER5, 0, 0};

typedef struct Input {
	int left, right, jump, mleft, mright;
} Input;

SDL_Surface *temp, *tileset, *player_image, *cursor;
Input input;

/* prototypes */
void out_of_bounds(int x, int y);

unsigned char get_cell_type(int x, int y);

void set_cell_type(int x, int y, unsigned char value);

unsigned char get_cell_direction(int x, int y);

void set_cell_direction(int x, int y, unsigned char value);

unsigned char get_cell_mass(int x, int y);

void set_cell_mass(int x, int y, unsigned char value);

unsigned char get_cell_calc(int x, int y);

void set_cell_calc(int x, int y, unsigned char value);

void save_map(void);

void load_map(void);

int solid(int x, int y);

int air_cell(int x, int y);

int sand_cell(int x, int y);

int water_cell(int x,int y);

int oil_cell(int x, int y);

int liquid_cell(int x, int y);

int player_on_liquid_cell(struct Player *p);

int player_speed(struct Player *p);

int player_fall_speed(struct Player *p);

int not_solid_above(struct Player *p);

void on_ground(struct Player *p);

void draw_background(SDL_Surface *scr);

void load_tileset(void);

void draw_cell(SDL_Surface *screen, int x, int y, int cell_x);

void draw_all_cells(SDL_Surface *screen);

void load_player_image(void);

void check_against_map(struct Player *p);

void center_camera(struct Player *p);

void move_player(struct Player *p);

void draw_player(SDL_Surface *screen, struct Player *p);

void destroy_cell(SDL_Surface *scr, int x, int y);

void place_cell(int x, int y, struct Player *p);

void place_and_destroy_cells(SDL_Surface *scr, int x, int y, struct Player *p);

int not_player_position(int x, int y, struct Player *p);

void load_cursor(void);

void draw_cursor(SDL_Surface *screen, int *x, int *y);

void draw_text(struct Player *p, SDL_Surface *screen, TTF_Font *font, SDL_Color color);

void draw_save_msg(struct Player *p, SDL_Surface *screen, TTF_Font *font, SDL_Color color);

void draw(SDL_Surface *screen, int *mouse_x, int *mouse_y, struct Player *p, TTF_Font *font, SDL_Color color, int sv_msg);

void delay(unsigned int *frame_limit);

Uint32 msg_event(Uint32 interval, void *param);

Uint32 flow_event(Uint32 delay, void *param);

void sometimes_fill_bottom_cell(int x, int y);

void sometimes_fill_left_cell(int x, int y);

void sometimes_fill_right_cell(int x, int y);

void update_cell_type(int x, int y);

void fill_left_or_right_cell(int x, int y);

void update_cells(void);

void clear_grid(void);


int main(int argc, char *argv[]) {
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
		printf("Can't initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);

	SDL_Surface *screen;
	SDL_Event event;
	TTF_Font *font;
	int i, done = 0, mouse_x = 0, mouse_y = 0, liquid_flow = 0;
	unsigned int frames = SDL_GetTicks() + FLIMIT;
	unsigned int *frame_limit;
	frame_limit = &frames;
	int *mouse_x_ptr, *mouse_y_ptr;
	mouse_x_ptr = &mouse_x;
	mouse_y_ptr = &mouse_y;
	int show_save_msg = 0;
	SDL_TimerID flow_timer;

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_DOUBLEBUF | SDL_HWSURFACE);
	//screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_DOUBLEBUF | SDL_FULLSCREEN);
	if(NULL == screen) {
		printf("Can't set video mode: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	/* set window title */
	SDL_WM_SetCaption("2D SIMULATION", NULL);

	/* disable cursor */
	SDL_ShowCursor(SDL_DISABLE);

	/* load new cursor */
	load_cursor();

	/* load the map and fill tiles array */
	load_map();

	/* load tileset */
	load_tileset();

	/* load player */
	load_player_image();

	/* setup font */
	TTF_Init();
	SDL_Color text_color = {255, 255, 255};
	font = TTF_OpenFont("media/fonts/slkscrb.ttf", 8);

	/* init flow event timer */
	flow_timer = SDL_AddTimer(30, flow_event, NULL);

	/* game loop */
	while(!done) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = 1;
					break;
				case SAVE_EVENT:
					show_save_msg = 0;
					break;
				case FLOW_EVENT:
					liquid_flow = 1;
					break;
				case SDL_MOUSEMOTION:
					*mouse_x_ptr = event.motion.x;
					*mouse_y_ptr = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(event.button.button) {
						case 1:
							input.mleft = 1;
							break;
						case 3:
							input.mright = 1;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(event.button.button) {
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
							player.selected = DIRT;
							break;
						case SDLK_2:
							player.selected = GRASS;
							break;
						case SDLK_3:
							player.selected = SAND;
							break;
						case SDLK_4:
							player.selected = ROCK;
							break;
						case SDLK_5:
							player.selected = WATER5;
							break;
						case SDLK_6:
							player.selected = OIL;
							break;
						case SDLK_F12:
							save_map();
							SDL_AddTimer(2000, msg_event, NULL);
							show_save_msg = 1;
							break;
						case SDLK_F11:
							clear_grid();
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
		place_and_destroy_cells(screen, event.button.x, event.button.y, &player);
		input.mleft = 0; // uncomment for click once to delete one cell
		//input.mright = 0; // uncomment for click once to place one cell

		if(liquid_flow) {
			update_cells();
			liquid_flow = 0;
		}

		draw(screen, mouse_x_ptr, mouse_y_ptr, &player, font, text_color, show_save_msg);

		delay(frame_limit);
		*frame_limit = SDL_GetTicks() + FLIMIT;
	}

	/* free cell array in reverse order */
	for(i = 0; i < map.h; i++) {
		free(map.grid[i]);
	}
	free(map.grid);

	/* free all surfaces */
	SDL_FreeSurface(tileset);
	SDL_FreeSurface(player_image);
	SDL_FreeSurface(cursor);

	return 0;
}


/* functions */

void out_of_bounds(int x, int y) {
	printf("x:%i|0-%i y:%i|0-%i\n", x, map.w-1, y, map.h-1);
}

unsigned char get_cell_type(int x, int y) {
	if(x >= 0 && y >= 0 && x < map.w && y < map.h) {
		return cell[map.grid[x][y]].cell_type;
	}
	else {
		out_of_bounds(x, y);
		exit(EXIT_FAILURE);
	}
}

void set_cell_type(int x, int y, unsigned char value) {
	cell[map.grid[x][y]].cell_type = value;
}

unsigned char get_cell_direction(int x, int y) {
	if(x >= 0 && y >= 0 && x < map.w && y < map.h) {
		return cell[map.grid[x][y]].direction;
	}
	else {
		out_of_bounds(x, y);
		exit(EXIT_FAILURE);
	}
}

void set_cell_direction(int x, int y, unsigned char value) {
	cell[map.grid[x][y]].direction = value;
}

unsigned char get_cell_mass(int x, int y) {
	if(x >= 0 && y >= 0 && x < map.w && y < map.h) {
		return cell[map.grid[x][y]].mass;
	}
	else {
		out_of_bounds(x, y);
		exit(EXIT_FAILURE);
	}
}

void set_cell_mass(int x, int y, unsigned char value) {
	cell[map.grid[x][y]].mass = value;
}

unsigned char get_cell_calc(int x, int y) {
	if(x >= 0 && y >= 0 && x < map.w && y < map.h) {
		return cell[map.grid[x][y]].calc;
	}
	else {
		out_of_bounds(x, y);
		exit(EXIT_FAILURE);
	}
}

void set_cell_calc(int x, int y, unsigned char value) {
	cell[map.grid[x][y]].calc = value;
}

void save_map(void) {
	FILE *mmap;
	int x, y;
	char tile;
	int p_x = player.x / map.cellsize, p_y = player.y / map.cellsize;

	/* open mapfile */
	mmap = fopen(map.filename, "w+");
	if(NULL == mmap) {
		printf("couldn't load map file\n");
		exit(EXIT_FAILURE);
	}

	/* store tile symbols to mapfile instead of internal used integers */
	for(y = 0; y < map.h; y++) {
		for(x = 0; x < map.w; x++) {
			switch(get_cell_type(x, y)) {
				case AIR:		tile = '.';	break;
				case DIRT:		tile = '#';	break;
				case GRASS:		tile = 'g';	break;
				case SAND:		tile = 's';	break;
				case ROCK:		tile = 'r';	break;
				case WATER1:	tile = '1';	break;
				case WATER2:	tile = '2';	break;
				case WATER3:	tile = '3';	break;
				case WATER4:	tile = '4';	break;
				case WATER5:	tile = '5';	break;
				case OIL:		tile = 'o';	break;
			}

			/* we must store the player position also */
			if((x == p_x) && (y == p_y)) {
				tile = 'p';
			}
			fprintf(mmap, "%c", tile);
		}
		if(y + 1 < map.h) {
			fprintf(mmap, "\n");
		}
	}
	fclose(mmap);
}

/* load map file and fill grid array */
void load_map(void) {
	int c, i, j, k, counter;
	unsigned char mass;
	i = j = k = counter = 0;
	int nRet;
	FILE *mmap;
	size_t *t = malloc(0);
	char **gptr = (char **)malloc(sizeof(char*));
	*gptr = NULL;

	/* open mapfile */
	mmap = fopen(map.filename, "r");
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

	/* allocate memory for grid array */
	map.grid = (int **)malloc(map.w * sizeof(int *));
	if(NULL == map.grid) {
		printf("not enough free ram (tiles)");
		exit(EXIT_FAILURE);
	}
	for(k = 0; k < map.w; k++) {
		map.grid[k] = (int *)malloc(map.h * sizeof(int));
		if(NULL == map.grid[k]) {
			printf("not enough free ram for (tiles)row: %d\n", k);
			exit(EXIT_FAILURE);
		}
	}

	/* reset position to begin of stream */
	rewind(mmap);

	/* fill grid array with index numbers from mapfile */
	while( (c = fgetc(mmap)) != EOF ) {
		/* if newline */
		if(c == 10) {
			j++;
			i = 0;
		}
		else {
			switch(c) {
				case '.':	c = AIR;		break;
				case '#':	c = DIRT;		break;
				case 'g':	c = GRASS;		break;
				case 's':	c = SAND;		break;
				case 'r':	c = ROCK;		break;
				case '1':	c = WATER1;		break;
				case '2':	c = WATER2;		break;
				case '3':	c = WATER3;		break;
				case '4':	c = WATER4;		break;
				case '5':	c = WATER5;		break;
				case 'o':	c = OIL;		break;
				case 'p':
					player.x = i * map.cellsize;
					player.y = j * map.cellsize;
					c = AIR;
					break;
			}
			map.grid[i][j] = counter;

			/* fill cell struct with cell states for every index in grid */
			cell[counter].cell_type = c;
			cell[counter].calc = TRUE;
			switch(c) {
				case WATER1:
					mass = 1;
					break;
				case WATER2:
					mass = 2;
					break;
				case WATER3:
					mass = 3;
					break;
				case WATER4:
					mass = 4;
					break;
				case WATER5:
					mass = 5;
					break;
				default:
					mass = 0;
					break;
			}
			cell[counter].mass = mass;
			cell[counter].direction = NONE;
			i++;
			counter++;
		}
	}
	fclose(mmap);

	/* set min and max scroll position of the map */
	map.min_x = map.min_y = 0;
	map.max_x = map.w * map.cellsize;
	map.max_y = map.h * map.cellsize;
}

/* cells containing anything but water, oil and air are solid */
int solid(int x, int y) {
	int dx = x/map.cellsize;
	int dy = y/map.cellsize;
	if((dx < map.w) && (dy < map.h)) { /* fix for trying to access index out of bounds > */
		if( (y < 0) || (x < 0) || (dy >= map.h) || (dx >= map.w) ||
				( !air_cell(dx, dy) && !liquid_cell(dx, dy))) {
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

int air_cell(int x, int y) {
	return get_cell_type(x, y) == AIR;
}

/* returns 1 if tile is on map and contains sand */
int sand_cell(int x, int y) {
	return get_cell_type(x, y) == SAND;
}

/* returns 1 if tile is on map and contains water */
int water_cell(int x,int y) {
	int ret = 0;
	switch(get_cell_type(x, y)) {
		case WATER1:
			ret = 1;
			break;
		case WATER2:
			ret = 1;
			break;
		case WATER3:
			ret = 1;
			break;
		case WATER4:
			ret = 1;
			break;
		case WATER5:
			ret = 1;
			break;
		default:
			break;
	}
	return ret;
}

/* returns 1 if tile is on map and contains oil */
int oil_cell(int x, int y) {
	return get_cell_type(x, y) == OIL;
}

int liquid_cell(int x, int y) {
	return (water_cell(x, y) || oil_cell(x, y));
}

int player_on_liquid_cell(struct Player *p) {
	if(liquid_cell(p->x/map.cellsize, (p->y + p->h) / map.cellsize - 1)) {
		return 1;
	}
	else {
		return 0;
	}
}

int player_speed(struct Player *p) {
	if(player_on_liquid_cell(p)) {
		return PLAYER_SPEED/2;
	}
	else {
		return PLAYER_SPEED;
	}
}

int player_fall_speed(struct Player *p) {
	if(player_on_liquid_cell(p)) {
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
	temp = IMG_Load("media/images/tileset_full.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY, SDL_MapRGB(temp->format, 255, 0, 0));
	/*SDL_SetAlpha(tileset, SDL_SRCALPHA, 170);*/
	tileset = SDL_DisplayFormat(temp);
	if(NULL == tileset) {
		printf("Can't load tileset: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

/* draw the cell */
void draw_cell(SDL_Surface *screen, int x, int y, int cell_x) {
	SDL_Rect src, dst;
	src.x = cell_x;
	src.y = 0;
	src.w = map.cellsize;
	src.h = map.cellsize;

	dst.x = x;
	dst.y = y;
	dst.w = map.cellsize;
	dst.h = map.cellsize;
	SDL_BlitSurface(tileset, &src, screen, &dst);
}

/* iterate through grid array and draw cells */
void draw_all_cells(SDL_Surface *screen) {
	int x, y, tileset_index;
	int x1, x2, y1, y2, map_x, map_y;

	map_x = map.min_x / map.cellsize;
	x1 = (map.min_x % map.cellsize) * -1;
	x2 = x1 + SCREEN_WIDTH + (x1 == 0 ? 0 : map.cellsize);

	map_y = map.min_y / map.cellsize;
	y1 = (map.min_y % map.cellsize) * -1;
	y2 = y1 + SCREEN_HEIGHT + (y1 == 0 ? 0 : map.cellsize);

	for(y = y1; y < y2; y += map.cellsize) {
		map_x = map.min_x / map.cellsize;
		for(x = x1; x < x2; x += map.cellsize) {
			tileset_index = get_cell_type(map_x, map_y) - 1;
			draw_cell(screen, x, y, tileset_index * map.cellsize);
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
					/* reset upward movement to prevent jumping if player hits above cell and then moves left/right */
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

/* destroy cell */
void destroy_cell(SDL_Surface *scr, int x, int y) {
	int dx = (x + map.min_x)/map.cellsize;
	int dy = (y + map.min_y)/map.cellsize;
	if((dx < map.w) && (dy < map.h)) {
		if(!air_cell(dx, dy)) {
			set_cell_type(dx, dy, AIR);
			set_cell_mass(dx, dy, 0);
			set_cell_direction(dx, dy, NONE);
			set_cell_calc(dx, dy, TRUE);
		}
	}
}

/* place cell if there is enough room */
void place_cell(int x, int y, struct Player *p) {
	int cam_x = x + map.min_x;
	int cam_y = y + map.min_y;
	int dx = cam_x/map.cellsize;
	int dy = cam_y/map.cellsize;

	if((dx < map.w) && (dy < map.h) && not_player_position(dx, dy, p) ) {
		/* not solid at new cell position */
		if(!solid(cam_x, cam_y)) {
			set_cell_type(dx, dy, p->selected);
			set_cell_mass(dx, dy, ((p->selected == WATER5) ? MAX_MASS : 0));
			set_cell_direction(dx, dy, NONE);
			set_cell_calc(dx, dy, TRUE);
		}
	}
}

void place_and_destroy_cells(SDL_Surface *scr, int x, int y, struct Player *p) {
	// if mouse button is pressed and player moves cursor out of the screen, x and y could be negative
	if((x > 0) && (y > 0)) {
		if(input.mleft) {
			destroy_cell(scr, x, y);
		}
		if(input.mright) {
			place_cell(x, y, p);
		}
	}
}

/* returns 1 if click position is not player position */
int not_player_position(int x, int y, struct Player *p) {
	int x1, x2, x3, x4, y1, y2, y3, y4;
	x1 = p->x / map.cellsize;
	x2 = ((p->x) + p->w / 2) / map.cellsize;
	x3 = ((p->x) + p->w - 1) / map.cellsize;
	x4 = x1 + 1;
	y1 = p->y / map.cellsize;
	y2 = ((p->y) + p->h / 2) / map.cellsize;
	y3 = ((p->y) + p->h - 1) / map.cellsize;
	y4 = y1 + 1;
	if( !((x == x1) && (y == y1)) && !((x == x2) && (y == y2)) && !((x == x3) && (y == y3)) &&
			!((x + 1 == x4) && (y == y4)) && !((x == x4) && (y + 1 == y4))) {
		return 1;
	}
	else {
		return 0;
	}
}

void load_cursor(void) {
	temp = IMG_Load("media/images/cursor.png");
	SDL_SetColorKey(temp, SDL_SRCCOLORKEY, SDL_MapRGB(temp->format, 255, 0, 0));
	SDL_SetAlpha(temp, SDL_SRCALPHA, 100);
	cursor = SDL_DisplayFormat(temp);
	if(NULL == cursor) {
		printf("Can't load cursor: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_FreeSurface(temp);
}

void draw_cursor(SDL_Surface *screen, int *x, int *y) {
	SDL_Rect src, dst;
	src.x = 0;
	src.y = 0;
	src.w = map.cellsize;
	src.h = map.cellsize;

	dst.x = *x;
	dst.y = *y;
	dst.w = map.cellsize;
	dst.h = map.cellsize;
	SDL_BlitSurface(cursor, &src, screen, &dst);
}

void draw_text(struct Player *p, SDL_Surface *screen, TTF_Font *font, SDL_Color color) {
	SDL_Rect rect = {0, 0, 0, 0};
	char item[8];
	switch(p->selected) {
	case DIRT:
		strcat(item," DIRT ");
		break;
	case GRASS:
		strcat(item," GRASS ");
		break;
	case SAND:
		strcat(item," SAND ");
		break;
	case ROCK:
		strcat(item," ROCK ");
		break;
	case WATER5:
		strcat(item," WATER ");
		break;
	case OIL:
		strcat(item," OIL ");
		break;
	}
	SDL_Color bgcolor = {0, 0, 0};
	SDL_BlitSurface(TTF_RenderText_Shaded(font, item, color, bgcolor), NULL, screen, &rect);
}

void draw_save_msg(struct Player *p, SDL_Surface *screen, TTF_Font *font, SDL_Color color) {
	SDL_Rect rect = {0, screen->h - TTF_FontHeight(font), 0, 0};
	SDL_Color bgcolor = {0, 0, 0};
	SDL_BlitSurface(TTF_RenderText_Shaded(font, " Map saved... ", color, bgcolor), NULL, screen, &rect);
}

void draw(SDL_Surface *screen, int *mouse_x, int *mouse_y, struct Player *p, TTF_Font *font, SDL_Color color, int sv_msg) {
	draw_background(screen);
	draw_all_cells(screen);
	draw_text(p, screen, font, color);
	if(sv_msg) {
		draw_save_msg(p, screen, font, color);
	}
	draw_player(screen, p);
	draw_cursor(screen, mouse_x, mouse_y);

	SDL_Flip(screen);
	SDL_Delay(1);
}

void delay(unsigned int *frame_limit) {
	unsigned int ticks = SDL_GetTicks();

	if (*frame_limit < ticks) {
		return;
	}

	if (*frame_limit > ticks + FLIMIT) {
		SDL_Delay(FLIMIT);
	}
	else {
		SDL_Delay(*frame_limit - ticks);
	}
}

Uint32 msg_event(Uint32 interval, void *param) {
	SDL_Event event;
	SDL_UserEvent userevent;

	/* init userevent */
	userevent.type = SAVE_EVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	/* init a new event */
	event.type = SAVE_EVENT;
	event.user = userevent;

	/* create new event */
	SDL_PushEvent(&event);

	/* returning 0 instead of interval, to stop timer */
	interval = 0;
	return interval;
}

Uint32 flow_event(Uint32 delay, void *param) {
	SDL_Event f_event;
	SDL_UserEvent f_userevent;

	/* init userevent */
	f_userevent.type = FLOW_EVENT;
	f_userevent.code = 0;
	f_userevent.data1 = NULL;
	f_userevent.data2 = NULL;

	/* init a new event */
	f_event.type = FLOW_EVENT;
	f_event.user = f_userevent;

	/* create new event */
	SDL_PushEvent(&f_event);

	/* return 0 instead of interval, to stop timer */
	return delay;
}

void sometimes_fill_bottom_cell(int x, int y) {
	unsigned char current_mass = get_cell_mass(x, y);
	unsigned char bottom_mass = get_cell_mass(x, y+1);

	if(bottom_mass < MAX_MASS && current_mass >= MIN_MASS) {
		set_cell_mass(x, y, --current_mass);
		set_cell_mass(x, y+1, ++bottom_mass);
		//set_cell_calc(x, y+1, FALSE);
	}

}

void sometimes_fill_left_cell(int x, int y) {
	unsigned char current_mass = get_cell_mass(x, y);
	unsigned char left_mass = get_cell_mass(x-1, y);

	if(left_mass < current_mass && get_cell_direction(x, y) != RIGHT && current_mass >= MIN_MASS) {
		set_cell_mass(x, y, --current_mass);
		set_cell_direction(x, y, LEFT);
		set_cell_mass(x-1, y, ++left_mass);
		set_cell_calc(x-1, y, FALSE);
	}
}

void sometimes_fill_right_cell(int x, int y) {
	unsigned char current_mass = get_cell_mass(x, y);
	unsigned char right_mass = get_cell_mass(x+1, y);

	if(right_mass < current_mass && get_cell_direction(x, y) != LEFT && current_mass >= MIN_MASS) {
		set_cell_mass(x, y, --current_mass);
		set_cell_direction(x, y, RIGHT);
		set_cell_mass(x+1, y, ++right_mass);
		set_cell_calc(x+1, y, FALSE);
	}
}

void update_cell_type(int x, int y) {
	unsigned char type;

	switch(get_cell_mass(x, y)) {
		case 0:	type = AIR;		break;
		case 1:	type = WATER1;	break;
		case 2:	type = WATER2;	break;
		case 3:	type = WATER3;	break;
		case 4:	type = WATER4;	break;
		case 5:	type = WATER5;	break;
	}
	set_cell_type(x, y, type);
}

/* if current cell has direction flag, flow into direction cell
 * if current cell has less (or same) mass than opposite direction cell */
void fill_left_or_right_cell(int x, int y) {
	int left_cell_mass, right_cell_mass;
	left_cell_mass = get_cell_mass(x-1, y);
	right_cell_mass = get_cell_mass(x+1, y);

	switch(get_cell_direction(x, y)) {
		case LEFT:
			if(left_cell_mass <= right_cell_mass) {
				sometimes_fill_left_cell(x, y);
			}
			else {
				set_cell_direction(x, y, RIGHT);
				sometimes_fill_right_cell(x, y);
			}
			break;
		case RIGHT:
			if(right_cell_mass <= left_cell_mass) {
				sometimes_fill_right_cell(x, y);
			}
			else {
				set_cell_direction(x, y, LEFT);
				sometimes_fill_left_cell(x, y);
			}
			break;
	}
}

void update_cells(void) {
	int x, y;

	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			/* within map */
			if((y > 0) && (x > 0) && (y+1 < map.h) && (x+1 < map.w)) {

				/* flow down into bottom cell */
				if(water_cell(x, y) && (water_cell(x, y+1) || air_cell(x, y+1)) && !(get_cell_mass(x, y+1) == MAX_MASS)) {
					if(get_cell_calc(x, y)) {
						sometimes_fill_bottom_cell(x, y);
					}
				}

				/* if direction flag is set and both neighbours are emtpy/not full, move into flow direction */
				if(get_cell_mass(x, y) > 0 && water_cell(x, y) && !air_cell(x, y+1) && get_cell_calc(x, y)) {
					fill_left_or_right_cell(x, y);
				}

				/* flow into left cell */
				if(get_cell_mass(x, y) > 0 && water_cell(x, y) && (water_cell(x-1, y) || air_cell(x-1, y))) {
					if(!(air_cell(x, y+1)) && get_cell_calc(x, y)) {
						sometimes_fill_left_cell(x, y);
					}
				}

				/* flow into right cell */
				if(get_cell_mass(x, y) > 0 && water_cell(x, y) && (water_cell(x+1, y) || air_cell(x+1, y))) {
					if(!(air_cell(x, y+1)) && get_cell_calc(x, y)) {
						sometimes_fill_right_cell(x, y);
					}
				}

				/* reset calc bit for next timestep */
				if(get_cell_calc(x, y) == FALSE) {
					set_cell_calc(x, y, TRUE);
				}
			}
		}
	}
	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			/* change cell_type in case water moved */
			if (water_cell(x, y) || air_cell(x, y)) {
				update_cell_type(x, y);
			}
		}
	}
}

/* delete all cells */
void clear_grid(void) {
	int x, y;
	for(x = 0; x < map.w; x++) {
		for(y = 0; y < map.h; y++) {
			set_cell_type(x, y, AIR);
			set_cell_direction(x, y, NONE);
			set_cell_mass(x, y, 0);
			set_cell_calc(x, y, TRUE);
		}
	}
}

