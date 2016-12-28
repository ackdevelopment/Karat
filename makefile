SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
CFLAGS=-Wall -Wextra -Werror -O3 -std=c11 -DKDEBUG -I./libs/include/ -L./libs
LIBS=-lSDL2 -lm
EXECUTABLE=karat

ifeq ($(OS), Windows_NT)
	EXECUTABLE = karat.exe
	RM = del
endif

$(EXECUTABLE) : $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

run:
	./$(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
	$(RM) *.o
