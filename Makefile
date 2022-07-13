NC     = \033[0m
YELLOW = \033[1;33m
GREEN  = \033[1;32m

CC = gcc
LD = gcc

CFLAGS = -std=c17 -Wall -Werror -pedantic -Iinclude
CFLAGS += -Ideps/pcre2/src
# Uncomment for release
# CFLAGS += -O3
# Uncomment for debug
CFLAGS += -O0 -ggdb

LDFLAGS = deps/pcre2/build/libpcre2-posix.a deps/pcre2/build/libpcre2-8.a

BIN = bin

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

EXE = keac

.PHONY: all deps clean

deps:
	cd deps/pcre2 && mkdir -p build && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DPCRE2_BUILD_TESTS=OFF && cmake --build . --config Release && make

all: $(EXE)

$(EXE): $(OBJ)
	@ mkdir -p $(BIN)
	@ echo -e "$(GREEN)LINKING$(NC) $(EXE)"
	@ $(LD) $^ -o $(BIN)/$@ $(LDFLAGS)

%.o: %.c
	@ echo -e "$(GREEN)COMPILING$(NC) $<"
	@ $(CC) $(CFLAGS) -c $< -o $@

clean:
	@ echo -e "$(YELLOW)CLEANING PROJECT$(NC)"
	@ rm -rf $(BIN) $(OBJ)
