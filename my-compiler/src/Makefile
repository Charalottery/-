CC = D:/msys64/mingw64/bin/g++.exe
CFLAGS = -g -Wall -std=c++17

SRCS = main.cpp frontend/lexer/lexer.cpp frontend/parser/Parser.cpp
TARGET = Compiler.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	-rm -f $(TARGET)
