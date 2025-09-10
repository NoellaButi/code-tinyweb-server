CC := gcc
CFLAGS := -Wall -Wextra -O2 -g
LDFLAGS := -pthread
TARGET := wserver

all: $(TARGET)

$(TARGET): src/wserver.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
