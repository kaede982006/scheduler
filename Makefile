CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -O2 -std=c11 -Iinclude
TARGET  = scheduler

OBJS    = src/main.o  \
          src/util.o  \
          src/task.o  \
          src/fmt1.o  \
          src/fmt2.o

HDRS    = include/util.h  \
          include/task.h  \
          include/fmt1.h  \
          include/fmt2.h

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "빌드 완료: ./$(TARGET)"

src/main.o: src/main.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

src/util.o: src/util.c include/util.h
	$(CC) $(CFLAGS) -c $< -o $@

src/task.o: src/task.c include/task.h include/util.h
	$(CC) $(CFLAGS) -c $< -o $@

src/fmt1.o: src/fmt1.c include/fmt1.h include/task.h include/util.h
	$(CC) $(CFLAGS) -c $< -o $@

src/fmt2.o: src/fmt2.c include/fmt2.h include/task.h include/util.h
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@mkdir -p $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/
	@echo "설치 완료: $(HOME)/.local/bin/$(TARGET)"

clean:
	rm -f $(OBJS) $(TARGET)
