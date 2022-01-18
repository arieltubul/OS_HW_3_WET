# Makefile for the ttftps program
CC = gcc
CFLAGS = -g -Wall -std=c99 -Werror -pedantic-errors -DNDEBUG 
CCLINK = $(CC)
OBJS =  ttftps.o
LFLAGS = -g 
RM = rm -f
# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) -o ttftps $(OBJS)
# Creating the object files

ttftps.o: ttftps.c ttftps.h

# Cleaning old files before new make
clean:
	$(RM) ttftps *.o *~ "#"* core.*
