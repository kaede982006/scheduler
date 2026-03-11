#ifndef TASK_H
#define TASK_H

/* ── 상수 ─────────────────────────────────────────────────────────────── */
#define MAX_TASKS  2048
#define TITLE_LEN   512   /* 경로가 길어질 수 있으므로 확장 */
#define DESC_LEN    512

/* ── Task 구조체 ──────────────────────────────────────────────────────── */
typedef struct {
    int  year, month, day;
    char title[TITLE_LEN];   /* 경로 형식: "루트" 또는 "루트/자식/손자" */
    char desc[DESC_LEN];
    int  repeat_days;        /* 0=반복 없음, N=N일마다 반복 (루트만 허용) */
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

/* ── 경로 유틸리티 ────────────────────────────────────────────────────── */

/*
 * path_depth: '/' 구분자 개수 반환 (0 = 루트 노드)
 *   "팀"         → 0
 *   "팀/회의"    → 1
 *   "팀/회의/일" → 2
 */
int  path_depth   (const char *title);

/*
 * path_basename: 마지막 '/' 이후의 이름(표시용 노드명) 반환
 *   "팀/회의" → "회의"
 *   "팀"      → "팀"
 */
void path_basename(const char *title, char *out, int cap);

/*
 * path_parent: 마지막 '/' 이전의 경로를 out 에 복사
 *   "팀/회의/일" → "팀/회의", 반환값 1
 *   "팀"         → "", 반환값 0 (루트: 부모 없음)
 */
int  path_parent  (const char *title, char *out, int cap);

/* ── 반복 판정 ────────────────────────────────────────────────────────── */
int task_occurs_on(int idx, int y, int m, int d);

/* ── 쿼리 ─────────────────────────────────────────────────────────────── */
int task_query_day    (int y, int m, int d, int out[], int cap);
int task_find_by_title(int y, int m, int d, const char *title);

/* ── CRUD ─────────────────────────────────────────────────────────────── */

/*
 * task_upsert: (날짜, 제목/경로) 기준 추가
 *   0  = 신규 추가
 *  -1  = 용량 초과
 *  -2  = 이미 존재 (중복)
 */
int task_upsert(int y, int m, int d, const char *title);

/*
 * task_del_by_title: (날짜, 경로) 일치 삭제
 *   부모 경로 삭제 시 하위 경로(자식)도 함께 cascade 삭제
 *   반환값 = 총 삭제 수
 */
int task_del_by_title(int y, int m, int d, const char *title);

/* task_del: 인덱스 단건 삭제. 성공 1, 범위 초과 0 */
int task_del(int idx);

/* ── 반복 설정 ────────────────────────────────────────────────────────── */
int task_set_repeat(int idx, int n);

/* ── 설명 ─────────────────────────────────────────────────────────────── */
int task_set_desc  (int idx, const char *desc);
int task_clear_desc(int idx);

/* ── 트리 렌더링 ──────────────────────────────────────────────────────── */
/*
 * task_render_tree: indices[] 의 task 를 트리 형식으로 stdout 출력
 *
 *   indices     : 해당 날짜에 발생하는 task 인덱스 배열 (title 기준 정렬)
 *   n           : 개수
 *   parent_path : 현재 렌더링 중인 부모 경로
 *                 루트 레벨이면 빈 문자열 ""
 *   indent      : 현재 들여쓰기 접두사
 */
void task_render_tree(const int *indices, int n,
                      const char *parent_path, const char *indent);

#endif /* TASK_H */
