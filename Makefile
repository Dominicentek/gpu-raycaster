CFLAGS := -g
LDFLAGS := -g -lGL -lGLEW -lglfw -lm

game: $(shell find -name "*.c" -or -name "*.h")
	gcc $^ $(CFLAGS) $(LDFLAGS) -o $@
	