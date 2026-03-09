CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -O2 -std=c11 -Iinclude
TARGET  = scheduler

OBJS    = src/main.o  \
          src/util.o  \
          src/task.o  \
          src/fmt1.o  \
          src/fmt2.o  \
          src/edit.o

HDRS    = include/util.h  \
          include/task.h  \
          include/fmt1.h  \
          include/fmt2.h  \
          include/edit.h

.PHONY: all clean install

# ── 최종 바이너리 링크 ─────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "빌드 완료: ./$(TARGET)"

# ── 명시적 컴파일 룰 (각 .o 별 헤더 의존성 포함) ──────────────────────
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

src/edit.o: src/edit.c include/edit.h include/task.h include/fmt2.h include/util.h
	$(CC) $(CFLAGS) -c $< -o $@

# ── 유틸 ───────────────────────────────────────────────────────────────
install: $(TARGET)
	@mkdir -p $(HOME)/.local/bin
	install -m 755 $(TARGET) $(HOME)/.local/bin/
	@echo "설치 완료: $(HOME)/.local/bin/$(TARGET)"

clean:
	rm -f $(OBJS) $(TARGET)
