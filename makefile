#all: main.o
#	gcc -o sim main.o `sdl-config --libs` -lSDL_image

#main.o: main.c
#	gcc -c main.c `sdl-config --cflags` -lSDL_image -pedantic -W -Wall
#	gcc -c main.c `sdl-config --cflags` -lSDL_image
	

CFLAGS = `sdl-config --cflags` -lSDL_image -lSDL_ttf
LFLAGS = `sdl-config --libs` -lSDL_image -lSDL_ttf
OBJS   = main.o
PROG = sim
CXX = gcc

# top-level rule to create the program.
all: $(PROG)

# compiling other source files.
%.o: %.c %.h
	$(CXX) $(CFLAGS) -c -s $<

# linking the program.
$(PROG): $(OBJS)
	$(CXX) $(OBJS) -o $(PROG) $(LFLAGS)

# cleaning everything that can be automatically recreated with "make".
clean:
	rm $(PROG) *.o