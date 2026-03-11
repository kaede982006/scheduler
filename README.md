# scheduler

GNU GCC / Linux 환경의 CLI 스케줄러.

---

## 빌드

```bash
make          # 또는 make all
make install  # ~/.local/bin/scheduler 에 설치
make clean
```

---

## 사용법

```
scheduler                                          # 이번 달 1형식 (= -M)
scheduler -M [YYYY-MM]                             # 1형식: 월간 달력 뷰
scheduler -D [YYYY-MM-DD]                          # 2형식: 일간 시간 뷰

scheduler -t YYYY-MM-DD HH:MM <제목> [HH:MM <제목> ...]   # 일정 추가/수정
scheduler -r YYYY-MM-DD <제목> [<제목> ...]               # 일정 삭제
scheduler -c YYYY-MM-DD HH:MM <제목> <N>                  # N일마다 반복 (0=해제)

scheduler -p YYYY-MM-DD HH:MM <제목>              # 설명 출력
scheduler -i YYYY-MM-DD HH:MM <제목> <설명>       # 설명 추가/수정
scheduler -d YYYY-MM-DD HH:MM <제목>              # 설명 삭제

scheduler -h                                       # 도움말
```

### 규칙

- 날짜 생략 시 오늘 날짜 기준
- `-t` : (날짜, 제목) 기준 upsert — 없으면 추가, 있으면 시각 수정
- 동일한 날짜 + 시각 + 제목은 중복으로 차단됨
- 각 옵션은 독립적으로 수행됨
- 설명은 1형식/2형식 뷰에 표시되지 않음
- 일정 삭제(`-r`) 시 설명도 함께 삭제됨

---

## 반복 일정 (`-c`)

```bash
# 2026-03-01부터 3일마다 반복
scheduler -c 2026-03-01 08:00 "데일리 체크" 3

# 반복 해제
scheduler -c 2026-03-01 08:00 "데일리 체크" 0
```

반복 일정은 뷰에서 `[↻N일]` 마커로 표시된다.

---

## 데이터 저장

```
~/.local/share/scheduler/tasks.json
```

---

## 프로젝트 구조

```
scheduler/
├── Makefile
├── README.md
├── include/
│   ├── util.h    ANSI/트리 기호, date_to_days 선언
│   ├── task.h    Task 구조체 (repeat_days, desc), 전체 API
│   ├── fmt1.h    1형식 렌더 API
│   └── fmt2.h    2형식 렌더 API
└── src/
    ├── main.c    옵션 파싱, 핸들러 디스패치
    ├── util.c    날짜 계산, date_to_days 구현
    ├── task.c    JSON 영속성, repeat 판정, CRUD
    ├── fmt1.c    월간 뷰 (repeat 마커 포함)
    └── fmt2.c    일간 뷰 (repeat 마커 포함)
```
