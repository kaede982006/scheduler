CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -O2 -std=c11 -Iinclude
TARGET  = scheduler

SRCS    = src/main.c   \
          src/util.c   \
          src/task.c   \
          src/fmt1.c   \
          src/fmt2.c   \
          src/edit.c

OBJS    = $(SRCS:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "빌드 완료: ./$(TARGET)"

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@mkdir -p $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/
	@echo "설치 완료: $(HOME)/.local/bin/$(TARGET)"

clean:
	rm -f $(OBJS) $(TARGET)
