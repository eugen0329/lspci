CC = gcc
CFLAGS = -std=c99 -Wall -pedantic-errors
LDFLAGS =

SRC = main.c

TARGET = bin

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
