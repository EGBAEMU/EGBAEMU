CC := g++
FLAGS := -std=c++17 -g -Wall
SRC := src
OUT := bin


all: $(SRC)/main.cpp
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -o $(OUT)/gbaemu $^
