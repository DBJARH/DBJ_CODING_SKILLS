CC      := gcc
CFLAGS  := -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau
TARGET  := dbj_email_crud

.PHONY: all run clean

all: $(TARGET)

$(TARGET): dbj_email_crud.c dbj_email_record.h dbj_email_storage_result.h dbj_email_storage.h ../toplevel/simple_log.h ../third_party/inifile/inifile.h dbj_email_crud.ini
	$(CC) $(CFLAGS) -o $(TARGET) dbj_email_crud.c

run: $(TARGET)
	./$(TARGET)

ifeq ($(OS),Windows_NT)
clean:
	del /f /q $(TARGET).exe 2>nul
else
clean:
	rm -f $(TARGET) $(TARGET).exe
endif
