
CC = D:/msys64/mingw64/bin/g++
# keep debug symbol by default; use `make CFLAGS="-std=c++17 -O2 -I ."` to build optimized
CFLAGS = -g -std=c++17 -I .
SRCS = main.cpp \
       frontend/FrontendDriver.cpp \
       frontend/lexer/lexer.cpp \
       frontend/parser/Parser.cpp \
       frontend/parser/ParserHelper.cpp \
       frontend/parser/ParserOutput.cpp \
       frontend/parser/ParserWriter.cpp \
       frontend/ast/CompUnit.cpp \
       frontend/ast/Node.cpp \
       frontend/ast/AST.cpp
TARGET = Compiler.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	@echo Running $(TARGET)
	@.

clean:
	-@if exist $(TARGET) del /Q $(TARGET)

.PHONY: all clean run
