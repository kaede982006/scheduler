#ifndef TASK_H
#define TASK_H

/* ── 상수 ─────────────────────────────────────────────────────────────── */
#define MAX_TASKS  2048
#define TITLE_LEN   256
#define DESC_LEN    512

/* ── Task 구조체 ──────────────────────────────────────────────────────── */
typedef struct {
    int  year, month, day;  /* 시작 날짜 */
    int  hour, minute;
    char title[TITLE_LEN];
    char desc[DESC_LEN];    /* 설명 (없으면 빈 문자열) */
    int  repeat_days;       /* 0=반복 없음, N=N일마다 반복 */
} Task;

/* ── 전역 저장소 ──────────────────────────────────────────────────────── */
extern Task tasks[];
extern int  n_tasks;

/* ── 영속성 ───────────────────────────────────────────────────────────── */
void task_init_path(void);
void task_load(void);
void task_save(void);

/* ── 정렬 ─────────────────────────────────────────────────────────────── */
void task_sort(void);

/* ── 반복 판정 ────────────────────────────────────────────────────────── */
/*
 * task_occurs_on: tasks[idx] 가 날짜 (y,m,d) 에 발생하는지 여부
 *   - repeat_days == 0: 정확히 시작 날짜와 일치할 때만 true
 *   - repeat_days  > 0: 시작 날짜 이후, 간격의 배수인 날에 true
 */
int task_occurs_on(int idx, int y, int m, int d);

/* ── 쿼리 ─────────────────────────────────────────────────────────────── */

/* 날짜 (y,m,d) 에 발생하는 task 인덱스를 out[]에 채워 반환 (반환값=개수) */
int task_query_day (int y, int m, int d, int out[], int cap);

/* 날짜+시각에 발생하는 task 인덱스를 out[]에 채워 반환 */
int task_query_hour(int y, int m, int d, int h, int out[], int cap);

/* (날짜, 제목) 으로 정확 날짜 매칭 첫 번째 인덱스 반환 (없으면 -1) */
int task_find_by_title(int y, int m, int d, const char *title);

/* (날짜, 시각, 제목) 으로 정확 매칭 첫 번째 인덱스 반환 (없으면 -1) */
int task_find_exact(int y, int m, int d, int hh, int mm, const char *title);

/* ── CRUD ─────────────────────────────────────────────────────────────── */

/*
 * task_upsert: (날짜, 제목) 기준으로 추가/수정
 *
 *   - (날짜, 제목) 이 이미 존재 → 시각을 (hh:mm) 으로 갱신
 *   - 없으면 신규 추가
 *
 *   반환값:
 *     0  = 신규 추가
 *     1  = 기존 항목 시각 수정
 *    -1  = 용량 초과
 *    -2  = 동일 날짜+시각+제목 중복 (추가 불가 / 수정 불필요)
 */
int task_upsert(int y, int m, int d, int hh, int mm, const char *title);

/* task_del_by_title: (날짜, 제목) 이 일치하는 모든 task 삭제. 반환값=삭제 수 */
int task_del_by_title(int y, int m, int d, const char *title);

/* task_del: idx 단건 삭제 (내부/편의용). 성공 1, 범위 초과 0 */
int task_del(int idx);

/* ── 반복 설정 ────────────────────────────────────────────────────────── */
/* task_set_repeat: tasks[idx].repeat_days = n. 성공 1, 범위 초과 0 */
int task_set_repeat(int idx, int n);

/* ── 설명 ─────────────────────────────────────────────────────────────── */
int task_set_desc  (int idx, const char *desc);
int task_clear_desc(int idx);

#endif /* TASK_H */
