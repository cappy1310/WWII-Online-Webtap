SRC = stb_image.c webtap.c
OBJ = stb_image.o webtap.o

all: image.x

image.x : $(OBJ)
	gcc -lm $(OBJ) -o $@ -lmxml -lpthread

%.o : %.c
	gcc -c $<

clean:
	rm *.o
