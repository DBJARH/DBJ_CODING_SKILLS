CFLAGS  := -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau
TARGET  := dbj_email_crud

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
else
CC      := gcc-15
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): dbj_email_crud.c dbj_email_record.h dbj_email_storage_result.h dbj_email_storage.h ../toplevel/simple_log.h ../third_party/inifile/inifile.h
	$(CC) $(CFLAGS) -o $(TARGET) dbj_email_crud.c

ifeq ($(OS),Windows_NT)
clean:
	del /f /q $(TARGET).exe 2>nul
else
clean:
	rm -f $(TARGET) $(TARGET).exe
endif
