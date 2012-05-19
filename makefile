SRC_DIR = source
SRC = $(SRC_DIR)/stb_image.c $(SRC_DIR)/webtap.c $(SRC_DIR)/webtap_timer.c
OBJ = $(SRC_DIR)/stb_image.o $(SRC_DIR)/webtap.o $(SRC_DIR)/webtap_timer.o
LIB = -lm -lmxml -ljson -lcurl -Wl,-Bsymbolic-functions -lpthread
FLAGS = -Wunused -DXML_OUTPUT -DJSON_OUTPUT

all: webtap

webtap: $(OBJ)
	clang -Wl, $(OBJ) -o $@ $(LIB) $(FLAGS)

debug: $(OBJ)
	clang $(OBJ) -o $@ $(LIB) -DDEBUG=1 $(FLAGS)

test: $(OBJ)
	clang $(OBJ) -o $@ $(LIB) -DLIBDIR=1 -Wl,libdir $(FLAGS)

%.o : %.c
	clang -c $< -o $@ -DDEBUG $(FLAGS)

clean:
	rm -f $(SRC_DIR)/*.o

purge:
	rm -f $(SRC_DIR)/*.o
	rm -f image.x
