#ifndef TASK_H
#define TASK_H

/* ── 상수 ─────────────────────────────────────────────────────────────── */
#define MAX_TASKS  2048
#define TITLE_LEN   256

/* ── Task 구조체 ──────────────────────────────────────────────────────── */
typedef struct {
    int  year, month, day;
    int  hour, minute;
    char title[TITLE_LEN];
} Task;

/* ── 전역 저장소 (task.c에서 정의) ───────────────────────────────────── */
extern Task tasks[];
extern int  n_tasks;

/* ── 영속성 ───────────────────────────────────────────────────────────── */
void task_init_path(void);          /* 데이터 경로 초기화 (main 최초 호출) */
void task_load(void);
void task_save(void);

/* ── 정렬 ─────────────────────────────────────────────────────────────── */
void task_sort(void);

/* ── 쿼리 ─────────────────────────────────────────────────────────────── */

/* 특정 날짜에 속하는 task 인덱스를 out[]에 채워 반환 (반환값=개수) */
int task_query_day (int y, int m, int d,
                    int out[], int cap);

/* 특정 날짜+시각에 속하는 task 인덱스를 out[]에 채워 반환 (반환값=개수) */
int task_query_hour(int y, int m, int d, int h,
                    int out[], int cap);

/* ── CRUD ─────────────────────────────────────────────────────────────── */

/*
 * task_add: tasks[]에 새 Task 추가 후 sort
 * 성공 시 새 Task의 배열 인덱스, 실패(용량 초과) 시 -1
 */
int  task_add(int y, int m, int d, int hh, int mm, const char *title);

/*
 * task_del: 인덱스 id의 Task 삭제 (범위 초과 시 0, 성공 시 1)
 */
int  task_del(int id);

/*
 * task_edit: 인덱스 id의 Task 수정 후 sort
 * 범위 초과 시 0, 성공 시 1
 */
int  task_edit(int id, int hh, int mm, const char *title);

#endif /* TASK_H */
