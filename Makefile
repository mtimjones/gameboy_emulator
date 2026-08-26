CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = gbe
SOURCES = main.c cpu.c memory.c ppu.c viewer.c input.c
LIBS = -lgdi32 -luser32

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
