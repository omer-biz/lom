PROJECT = junbug
SRC = main.c
OBJ = $(SRC:.c=.o)

CC = gcc
CFLAGS = -Wall -O2 $(shell pkg-config --cflags lua5.4)
LDFLAGS = $(shell pkg-config --libs lua5.4)

all: $(PROJECT)

$(PROJECT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(PROJECT)

run: $(PROJECT)
	./$(PROJECT)
