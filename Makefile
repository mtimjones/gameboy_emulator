CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = gbe
SOURCES = main.c cpu.c memory.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean
