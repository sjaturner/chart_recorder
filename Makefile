CC=gcc
CFLAGS=-g -O3 -Wall -Wextra -Wimplicit-fallthrough=0 -fsanitize=address -fno-omit-frame-pointer
.PHONY: all
all: chart_recorder
.PHONY: clean
clean:
	rm -f chart_recorder
