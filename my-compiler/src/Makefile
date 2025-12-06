# Recursive Makefile for my-compiler
# Builds all .cpp under src and subdirectories into a single executable

CXX := g++
CXXFLAGS := -std=c++17 -O2 -g -Wall -I.
LDFLAGS :=

# Directories to search for .cpp files (relative to this Makefile)
SRCDIRS := . ./backend ./error ./frontend ./midend ./optimize ./tools ./utils

## Use PowerShell to emit relative paths (no drive letters) to avoid Make parsing
SRC := $(shell powershell -NoProfile -Command "Get-ChildItem -Recurse -Filter '*.cpp' | ForEach-Object { $$p=$$PWD.Path; $$r=$$_.FullName.Substring($$p.Length+1); $$r -replace '\\\\','/' }")

OUT := Compiler.exe

.PHONY: all clean run help print-vars

# Build by passing all .cpp files directly to the linker/driver so no .o files
# are left behind. This produces a single `Compiler.exe` in the current dir.
all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	@powershell -NoProfile -Command "Remove-Item -LiteralPath '$(OUT)' -Force -ErrorAction SilentlyContinue"

run: all
	@powershell -NoProfile -Command "& './$(OUT)'"

help: print-vars

print-vars:
	@echo "SRCDIRS: $(SRCDIRS)"
	@echo "Found sources:" 
	@printf "  %s\n" $(SRC)
	@echo "Output: $(OUT)"