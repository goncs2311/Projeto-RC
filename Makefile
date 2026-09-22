CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11

TARGET = user
SOURCE = user.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) -m 58000

clean:
	rm -f $(TARGET)