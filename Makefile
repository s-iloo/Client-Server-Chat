CC = gcc
CFLAGS = -Wall -Werror -g -pedantic
TALK = mytalk mytalk.o
LIB = -ltalk -lncurses


all: mytalk

mytalk: compile
	$(CC) $(CFLAGS) -L ~pn-cs357/Given/Talk/lib64 -o mytalk mytalk.o $(LIB)

compile: mytalk.c
	$(CC) $(CFLAGS) -c -I ~pn-cs357/Given/Talk/include mytalk.c

clean: 
	rm -f mytalk a.out
