CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g
LDFLAGS = -lncurses

SRCS    = src/main.c src/proc/cpu.c src/proc/mem.c src/proc/proc.c src/tui/ui.c src/utils/hmap.c
OBJS    = $(SRCS:.c=.o)
TARGET  = sysmon

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
