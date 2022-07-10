NC     = \033[0m
YELLOW = \033[1;33m
GREEN  = \033[1;32m

CC = gcc
LD = gcc

CFLAGS = -std=c17 -Wall -Werror -pedantic -Iinclude
# Uncomment for release
# CFLAGS += -O3
# Uncomment for debug
CFLAGS += -O0 -ggdb

BIN = bin

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

EXE = keac

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	@ mkdir -p $(BIN)
	@ echo -e "$(GREEN)LINKING$(NC) $(EXE)"
	@ $(LD) $(LDFLAGS) $^ -o $(BIN)/$@

%.o: %.c
	@ echo -e "$(GREEN)COMPILING$(NC) $<"
	@ $(CC) $(CFLAGS) -c $< -o $@

clean:
	@ echo -e "$(YELLOW)CLEANING PROJECT$(NC)"
	@ rm -rf $(BIN) $(OBJ)
