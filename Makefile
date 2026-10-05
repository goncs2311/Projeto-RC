CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11
CPPFLAGS = -Iinclude

TARGET = user

SOURCES = src/user.c src/handlers.c src/network.c src/validation.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) -m 58000

clean:
	rm -f $(OBJECTS) $(TARGET)