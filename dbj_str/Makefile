CFLAGS  := -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau
TARGET  := dbj_str_test

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
else
CC      := gcc-15
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): dbj_str_test.c dbj_str.h
	$(CC) $(CFLAGS) -o $(TARGET) dbj_str_test.c

run: $(TARGET)
	./$(TARGET)

ifeq ($(OS),Windows_NT)
clean:
	del /f /q $(TARGET).exe 2>nul
else
clean:
	rm -f $(TARGET) $(TARGET).exe
endif
