SOURCES=$(wildcard ./src/*.c)
OBJECTS=$(SOURCES:.c=.o)
ifndef CFLAGS
CFLAGS=-Wall -Wextra -Werror -Wno-unused-function -std=gnu99 -DKDEBUG -I./include/
endif
EXECUTABLE=karat
# check and see if we have ccache
HAS_CCACHE := $(shell command -v ccache 2> /dev/null)
ifdef HAS_CCACHE
	CC=ccache cc
else
	CC=cc
endif

ifeq ($(OS), Windows_NT)
	EXECUTABLE = karat.exe
	RM = del
endif


$(EXECUTABLE) : $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

generate:
	lua ./opgen/gen.lua > ./include/ops/opcodes.inc

profile:
	$(MAKE) clean CFLAGS='$(CFLAGS) -pg -O2' CC=gcc-4.9
	$(MAKE) CFLAGS='$(CFLAGS) -pg -O2' CC=gcc-4.9

run:
	./$(EXECUTABLE) ./test/test.k

tst:
	find ./test/*.k | xargs -n 1 ./$(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
	$(RM) $(OBJECTS)
