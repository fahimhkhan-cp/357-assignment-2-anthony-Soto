

CC = gcc
CFLAGS = -Wall -std=c99 -pedantic -g
MAIN = fssim
OBJS = fssim.c fssim.o

all: $(MAIN)

$(MAIN) :

fssim.o : fssim.c
	$(CC) $(CFLAGS) -c fssim.c

clean : 
	rm *.o $(MAIN)
