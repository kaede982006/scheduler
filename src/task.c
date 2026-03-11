#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "util.h"
#include "task.h"

/* ═══════════════════════════════════════════════════════════════════════
 * 전역 저장소
 * ═══════════════════════════════════════════════════════════════════════ */
Task tasks[MAX_TASKS];
int  n_tasks = 0;

static char data_file[512];

/* ═══════════════════════════════════════════════════════════════════════
 * 경로 초기화
 * ═══════════════════════════════════════════════════════════════════════ */
void task_init_path(void)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    char buf[480];
    snprintf(buf, sizeof buf, "%s/.local",                 home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share",           home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share/scheduler", home); mkdir(buf, 0755);
    snprintf(data_file, sizeof data_file,
             "%s/.local/share/scheduler/tasks.json", home);
}

/* ═══════════════════════════════════════════════════════════════════════
 * 경로 유틸리티
 * ═══════════════════════════════════════════════════════════════════════ */
int path_depth(const char *title)
{
    int d = 0;
    for (; *title; title++)
        if (*title == '/') d++;
    return d;
}

void path_basename(const char *title, char *out, int cap)
{
    const char *last = strrchr(title, '/');
    snprintf(out, cap, "%s", last ? last + 1 : title);
}

int path_parent(const char *title, char *out, int cap)
{
    const char *last = strrchr(title, '/');
    if (!last) { out[0] = '\0'; return 0; }
    int len = (int)(last - title);
    if (len >= cap) len = cap - 1;
    memcpy(out, title, (size_t)len);
    out[len] = '\0';
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 정렬
 * ═══════════════════════════════════════════════════════════════════════ */
static int cmp_task(const void *a, const void *b)
{
    const Task *x = a, *y = b;
    if (x->year  != y->year)  return x->year  - y->year;
    if (x->month != y->month) return x->month - y->month;
    if (x->day   != y->day)   return x->day   - y->day;
    return strcmp(x->title, y->title);
}

void task_sort(void)
{
    qsort(tasks, (size_t)n_tasks, sizeof(Task), cmp_task);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JSON 유틸리티
 * ═══════════════════════════════════════════════════════════════════════ */
static void json_write_str(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        switch (*s) {
        case '"':  fputs("\\\"", f); break;
        case '\\': fputs("\\\\", f); break;
        case '\n': fputs("\\n",  f); break;
        case '\r': fputs("\\r",  f); break;
        case '\t': fputs("\\t",  f); break;
        default:   fputc(*s, f);     break;
        }
    }
    fputc('"', f);
}

static int json_get_int(const char *line, const char *key, int *out)
{
    char pat[80];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ') p++;
    return (sscanf(p, "%d", out) == 1);
}

static int json_get_str(const char *line, const char *key, char *out, int cap)
{
    char pat[80];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ') p++;
    if (*p != '"') return 0;
    p++;
    int i = 0;
    while (*p && i < cap - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
            case '"':  out[i++] = '"';  break;
            case '\\': out[i++] = '\\'; break;
            case 'n':  out[i++] = '\n'; break;
            case 'r':  out[i++] = '\r'; break;
            case 't':  out[i++] = '\t'; break;
            default:   out[i++] = *p;   break;
            }
        } else if (*p == '"') {
            break;
        } else {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 로드 / 저장 (JSON)
 *
 * 파일 형식:
 * [
 *     {
 *         "year": 2026,
 *         "month": 3,
 *         "day": 9,
 *         "repeat_days": 0,
 *         "title": "루트/자식",
 *         "desc": ""
 *     }
 * ]
 * ═══════════════════════════════════════════════════════════════════════ */
void task_load(void)
{
    FILE *f = fopen(data_file, "r");
    if (!f) return;

    char line[2048];
    Task cur;
    int  in_obj = 0;
    n_tasks = 0;

    while (fgets(line, sizeof line, f) && n_tasks <= MAX_TASKS) {
        const char *tr = line;
        while (*tr == ' ' || *tr == '\t') tr++;

        if (*tr == '{') { memset(&cur, 0, sizeof cur); in_obj = 1; continue; }
        if (*tr == '}') {
            if (in_obj && n_tasks < MAX_TASKS) tasks[n_tasks++] = cur;
            in_obj = 0; continue;
        }
        if (!in_obj) continue;

        json_get_int(line, "year",        &cur.year);
        json_get_int(line, "month",       &cur.month);
        json_get_int(line, "day",         &cur.day);
        json_get_int(line, "repeat_days", &cur.repeat_days);
        json_get_str(line, "title",        cur.title, TITLE_LEN);
        json_get_str(line, "desc",         cur.desc,  DESC_LEN);
    }
    fclose(f);
    task_sort();
}

void task_save(void)
{
    task_sort();
    FILE *f = fopen(data_file, "w");
    if (!f) { perror("task_save: fopen"); return; }

    fprintf(f, "[\n");
    for (int i = 0; i < n_tasks; i++) {
        Task *t = &tasks[i];
        fprintf(f, "    {\n");
        fprintf(f, "        \"year\": %d,\n",        t->year);
        fprintf(f, "        \"month\": %d,\n",       t->month);
        fprintf(f, "        \"day\": %d,\n",         t->day);
        fprintf(f, "        \"repeat_days\": %d,\n", t->repeat_days);
        fprintf(f, "        \"title\": ");   json_write_str(f, t->title); fprintf(f, ",\n");
        fprintf(f, "        \"desc\": ");    json_write_str(f, t->desc);  fprintf(f, "\n");
        fprintf(f, "    }%s\n", (i < n_tasks - 1) ? "," : "");
    }
    fprintf(f, "]\n");
    fclose(f);
}

/* ═══════════════════════════════════════════════════════════════════════
 * 반복 판정
 * ═══════════════════════════════════════════════════════════════════════ */
int task_occurs_on(int idx, int y, int m, int d)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    Task *t = &tasks[idx];
    if (t->repeat_days <= 0)
        return (t->year == y && t->month == m && t->day == d);
    long diff = date_to_days(y, m, d) - date_to_days(t->year, t->month, t->day);
    return (diff >= 0 && diff % t->repeat_days == 0);
}

/* ═══════════════════════════════════════════════════════════════════════
 * 쿼리
 * ═══════════════════════════════════════════════════════════════════════ */
int task_query_day(int y, int m, int d, int out[], int cap)
{
    int cnt = 0;
    for (int i = 0; i < n_tasks && cnt < cap; i++)
        if (task_occurs_on(i, y, m, d))
            out[cnt++] = i;
    return cnt;
}

int task_find_by_title(int y, int m, int d, const char *title)
{
    for (int i = 0; i < n_tasks; i++)
        if (tasks[i].year  == y && tasks[i].month == m &&
            tasks[i].day   == d && strcmp(tasks[i].title, title) == 0)
            return i;
    return -1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * task_upsert
 * ═══════════════════════════════════════════════════════════════════════ */
int task_upsert(int y, int m, int d, const char *title)
{
    if (task_find_by_title(y, m, d, title) >= 0) return -2;
    if (n_tasks >= MAX_TASKS) return -1;

    Task *t        = &tasks[n_tasks++];
    t->year        = y; t->month = m; t->day = d;
    t->repeat_days = 0;
    snprintf(t->title, TITLE_LEN, "%s", title);
    t->desc[0]     = '\0';

    task_sort();
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 삭제 (cascade)
 *
 * title 과 정확히 일치하는 항목, 그리고 title + "/" 로 시작하는
 * 모든 하위 경로를 함께 삭제한다.
 * ═══════════════════════════════════════════════════════════════════════ */
int task_del(int idx)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    memmove(&tasks[idx], &tasks[idx + 1],
            (size_t)(n_tasks - idx - 1) * sizeof(Task));
    n_tasks--;
    return 1;
}

int task_del_by_title(int y, int m, int d, const char *title)
{
    /* cascade 접두사: "title/" */
    char prefix[TITLE_LEN + 2];
    snprintf(prefix, sizeof prefix, "%s/", title);
    int  plen   = (int)strlen(prefix);
    int  removed = 0;

    for (int i = n_tasks - 1; i >= 0; i--) {
        if (tasks[i].year != y || tasks[i].month != m || tasks[i].day != d)
            continue;
        if (strcmp(tasks[i].title, title) == 0 ||
            strncmp(tasks[i].title, prefix, (size_t)plen) == 0) {
            task_del(i);
            removed++;
        }
    }
    return removed;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 반복 설정 / 설명
 * ═══════════════════════════════════════════════════════════════════════ */
int task_set_repeat(int idx, int n)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    tasks[idx].repeat_days = n; return 1;
}
int task_set_desc(int idx, const char *desc)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    snprintf(tasks[idx].desc, DESC_LEN, "%s", desc); return 1;
}
int task_clear_desc(int idx)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    tasks[idx].desc[0] = '\0'; return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * task_render_tree
 *
 * 알고리즘:
 *   1. indices[] 중 parent_path 의 직계 자식 segment 를 수집 (중복 제거)
 *   2. 각 segment 를 순서대로 출력 (├── / └──)
 *   3. 해당 segment 의 full_path 에 task 가 존재하면 repeat 마커 부착
 *   4. 재귀적으로 그 segment 의 자식을 렌더링
 *
 * 정렬 보장: task_sort() 로 title 기준 알파벳 정렬 → 수집 순서 = 알파벳 순
 * ═══════════════════════════════════════════════════════════════════════ */
void task_render_tree(const int *indices, int n,
                      const char *parent_path, const char *indent)
{
    int parent_len = (int)strlen(parent_path);

    /* ── 직계 자식 segment 수집 ─────────────────────────────────────── */
    /* 최대 깊이/개수를 고려해 VLA 없이 고정 배열 사용 */
    char segs[MAX_TASKS][TITLE_LEN];
    int  n_segs = 0;

    for (int i = 0; i < n; i++) {
        const char *title = tasks[indices[i]].title;
        const char *rel   = title;

        /* parent_path 접두사 확인 */
        if (parent_len > 0) {
            if (strncmp(title, parent_path, (size_t)parent_len) != 0 ||
                title[parent_len] != '/')
                continue;
            rel = title + parent_len + 1;
        }

        /* rel 에서 첫 번째 segment 추출 (추가 '/' 이전까지) */
        char seg[TITLE_LEN];
        const char *slash = strchr(rel, '/');
        if (slash) {
            int len = (int)(slash - rel);
            if (len >= TITLE_LEN) len = TITLE_LEN - 1;
            memcpy(seg, rel, (size_t)len);
            seg[len] = '\0';
        } else {
            snprintf(seg, TITLE_LEN, "%s", rel);
        }

        /* 중복 제거 */
        int dup = 0;
        for (int j = 0; j < n_segs; j++)
            if (strcmp(segs[j], seg) == 0) { dup = 1; break; }
        if (!dup && n_segs < MAX_TASKS)
            snprintf(segs[n_segs++], TITLE_LEN, "%s", seg);
    }

    /* ── 각 segment 출력 + 재귀 ─────────────────────────────────────── */
    for (int si = 0; si < n_segs; si++) {
        int         is_last = (si == n_segs - 1);
        const char *pfx     = is_last ? T_END : T_MID;
        const char *cont    = is_last ? T_SPC : T_CON;

        /* full_path 구성 */
        char full_path[TITLE_LEN];
        if (parent_len > 0)
            snprintf(full_path, TITLE_LEN, "%s/%s", parent_path, segs[si]);
        else
            snprintf(full_path, TITLE_LEN, "%s", segs[si]);

        /* 해당 full_path 의 task 인덱스 검색 (반복 마커용) */
        int task_idx = -1;
        for (int j = 0; j < n; j++)
            if (strcmp(tasks[indices[j]].title, full_path) == 0) {
                task_idx = indices[j]; break;
            }

        /* 반복 마커 */
        char rm[48] = "";
        if (task_idx >= 0 && tasks[task_idx].repeat_days > 0)
            snprintf(rm, sizeof rm,
                     C_DIM " [↻%d일]" C_RESET, tasks[task_idx].repeat_days);

        /* 설명 마커 (설명이 있으면 [메모] 표시) */
        char dm[32] = "";
        if (task_idx >= 0 && tasks[task_idx].desc[0] != '\0')
            snprintf(dm, sizeof dm, C_DIM " [메모]" C_RESET);

        /* 노드 출력 */
        printf("%s%s" C_GREEN "%s" C_RESET "%s%s\n",
               indent, pfx, segs[si], rm, dm);

        /* 재귀: 자식 렌더링 */
        char new_indent[1024];
        snprintf(new_indent, sizeof new_indent, "%s%s", indent, cont);
        task_render_tree(indices, n, full_path, new_indent);
    }
}
