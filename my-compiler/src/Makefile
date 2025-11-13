CC = D:/msys64/mingw64/bin/g++.exe
CFLAGS = -g -Wall -std=c++17

SRCS = main.cpp frontend/lexer/lexer.cpp frontend/parser/Parser.cpp \
	midend/SymbolManager.cpp midend/SemanticAnalyzer.cpp
TARGET = Compiler.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Cross-platform clean: use rm on POSIX shells, and cmd del on Windows
ifeq ($(OS),Windows_NT)
RM_CMD = cmd /C if exist "$(TARGET)" del /F /Q "$(TARGET)"
else
RM_CMD = rm -f $(TARGET)
endif

clean:
	@$(RM_CMD)
