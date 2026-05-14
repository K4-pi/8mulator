
all: compile

compile:
	gcc main.c -o 8mulator src/* -Wall -O2 -lSDL3
