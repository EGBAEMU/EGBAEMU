CC := gcc
FLAGS := -std=c11
LINKS := -lwiringPi

all: src/virtualKeyboard.c
	mkdir -p bin
	$(CC) $(FLAGS) -o bin/virtualKeyboard $^ $(LINKS)
