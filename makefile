SRC_DIR = source
SRC = $(SRC_DIR)/stb_image.c $(SRC_DIR)/webtap.c
OBJ = $(SRC_DIR)/stb_image.o $(SRC_DIR)/webtap.o

all: image.x

image.x : $(OBJ)
	gcc -lm $(OBJ) -o $@ -lmxml -lpthread

%.o : %.c
	gcc -c $< -o $@

test:
	gcc -lm $(OBJ) -o $@ -lmxml -lpthread -LLIBDIR -Wl,libdir

clean:
	rm -f $(SRC_DIR)/*.o

purge:
	rm -f $(SRC_DIR)/*.o
	rm -f image.x
