#makefile fot ttftps
CC=g++
CFLAGS=-std=c++11 -g -Wall -Werror -pedantic-errors -DNDEBUG 
CCLINK=$(CC)
OBJS=ttftps.o
RM=rm -rf
TARGET=ttftps

$(TARGET): $(OBJS)
	$(CCLINK) $(CFLAGS) -o $(TARGET) $(OBJS)

ttftps.o: ttftps.cpp ttftps.h

clean:
	$(RM) $(OBJS) $(TARGET)
