NCURSES_DIR = $(HOME)/built_pckgs/ncurses-6.4

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g \
          -Isrc -Isrc/proc -Isrc/tui -Isrc/utils \
          -I$(NCURSES_DIR)/include/ncurses
LDFLAGS_STATIC = -static -L$(NCURSES_DIR)/lib -lncurses -ltinfo
LDFLAGS_SHARED = -lncurses

SRCS    = src/main.c src/proc/cpu.c src/proc/mem.c src/proc/proc.c \
          src/tui/ui.c src/utils/hmap.c src/hw/hw.c
OBJS    = $(SRCS:.c=.o)
TARGET  = sysmon

all: $(TARGET)

# statically linked against your built ncurses
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_STATIC) -o $@

# dynamically linked against the system ncurses .so
shared: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_SHARED) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all shared clean
