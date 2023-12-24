CC     := gcc
FLAGS  := -Wall -Wextra -std=c17
LIBS   := -lraylib -lm
TARGET := main

all: clean build

build: *.c
	$(CC) $(FLAGS) $(LIBS) -I./ -o $(TARGET) $^

clean:
	rm -f $(TARGET)

