CC = gcc
CCFLAGS =
LDFLAGS =

SRC = main.c

TARGET = bin


all:
	$(CC) -o $(TARGET) $(SRC)
