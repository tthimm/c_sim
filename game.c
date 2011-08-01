#include <stdio.h>
#include <stdlib.h>

#define WIDTH 16
#define HEIGHT 12

int tiles[WIDTH][HEIGHT];

void load_map(void) {
	int i = 0;
	int j = 0;
	int  c;
	FILE *map;

	map = fopen("world.map", "r");
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

int solid(int x, int y) {
	if(tiles[x][y] == '0') {
		return 0;
	}
	else {
		return 1;
	}
}

int main(void) {
	int coord;
	load_map();
	//coord = solid(WIDTH-1, HEIGHT-1);
	printf("%c solid? => %i\n", tiles[WIDTH-1][HEIGHT-1], solid(WIDTH-1, HEIGHT-1));
	printf("%c solid? => %i\n", tiles[0][0], solid(0, 0));

	return EXIT_SUCCESS;
}

