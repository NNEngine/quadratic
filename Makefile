CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared
LUA_INCLUDE = /usr/include/lua5.4

TARGET = quadratic.so
SRC = src/quadratic.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -I$(LUA_INCLUDE) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
