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
    snprintf(buf, sizeof buf, "%s/.local",                home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share",          home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share/scheduler",home); mkdir(buf, 0755);
    snprintf(data_file, sizeof data_file,
             "%s/.local/share/scheduler/tasks.json", home);
}

/* ═══════════════════════════════════════════════════════════════════════
 * 정렬
 * ═══════════════════════════════════════════════════════════════════════ */
static int cmp_task(const void *a, const void *b)
{
    const Task *x = a, *y = b;
    if (x->year   != y->year)   return x->year   - y->year;
    if (x->month  != y->month)  return x->month  - y->month;
    if (x->day    != y->day)    return x->day    - y->day;
    if (x->hour   != y->hour)   return x->hour   - y->hour;
    return x->minute - y->minute;
}

void task_sort(void)
{
    qsort(tasks, (size_t)n_tasks, sizeof(Task), cmp_task);
}

/* ═══════════════════════════════════════════════════════════════════════
 * JSON 유틸리티
 * ═══════════════════════════════════════════════════════════════════════ */

/* 문자열 JSON 이스케이프 출력 */
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

/* 행에서 정수값 추출: "key": N */
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

/* 행에서 문자열값 추출: "key": "value" — JSON unescape 포함 */
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
        if (*p == '\\' && *(p+1)) {
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
 * 로드 (JSON)
 *
 * 형식:
 * [
 *   {
 *     "year": 2026,
 *     "month": 1,
 *     "day": 1,
 *     "hour": 9,
 *     "minute": 0,
 *     "repeat_days": 0,
 *     "title": "...",
 *     "desc": "..."
 *   }
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
        /* 선행 공백 제거 후 첫 문자로 객체 경계 판정 */
        const char *tr = line;
        while (*tr == ' ' || *tr == '\t') tr++;

        if (*tr == '{') {
            memset(&cur, 0, sizeof cur);
            in_obj = 1;
            continue;
        }
        if (*tr == '}') {
            if (in_obj && n_tasks < MAX_TASKS)
                tasks[n_tasks++] = cur;
            in_obj = 0;
            continue;
        }
        if (!in_obj) continue;

        json_get_int(line, "year",        &cur.year);
        json_get_int(line, "month",       &cur.month);
        json_get_int(line, "day",         &cur.day);
        json_get_int(line, "hour",        &cur.hour);
        json_get_int(line, "minute",      &cur.minute);
        json_get_int(line, "repeat_days", &cur.repeat_days);
        json_get_str(line, "title",       cur.title, TITLE_LEN);
        json_get_str(line, "desc",        cur.desc,  DESC_LEN);
    }
    fclose(f);
    task_sort();
}

/* ═══════════════════════════════════════════════════════════════════════
 * 저장 (JSON)
 * ═══════════════════════════════════════════════════════════════════════ */
void task_save(void)
{
    task_sort();
    FILE *f = fopen(data_file, "w");
    if (!f) { perror("task_save: fopen"); return; }

    fprintf(f, "[\n");
    for (int i = 0; i < n_tasks; i++) {
        Task *t = &tasks[i];
        fprintf(f, "  {\n");
        fprintf(f, "    \"year\": %d,\n",        t->year);
        fprintf(f, "    \"month\": %d,\n",       t->month);
        fprintf(f, "    \"day\": %d,\n",         t->day);
        fprintf(f, "    \"hour\": %d,\n",        t->hour);
        fprintf(f, "    \"minute\": %d,\n",      t->minute);
        fprintf(f, "    \"repeat_days\": %d,\n", t->repeat_days);
        fprintf(f, "    \"title\": ");
        json_write_str(f, t->title);
        fprintf(f, ",\n");
        fprintf(f, "    \"desc\": ");
        json_write_str(f, t->desc);
        fprintf(f, "\n");
        fprintf(f, "  }%s\n", (i < n_tasks - 1) ? "," : "");
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

    if (t->repeat_days <= 0) {
        return (t->year == y && t->month == m && t->day == d);
    }
    long start  = date_to_days(t->year, t->month, t->day);
    long target = date_to_days(y, m, d);
    long diff   = target - start;
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

int task_query_hour(int y, int m, int d, int h, int out[], int cap)
{
    int cnt = 0;
    for (int i = 0; i < n_tasks && cnt < cap; i++)
        if (task_occurs_on(i, y, m, d) && tasks[i].hour == h)
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

int task_find_exact(int y, int m, int d, int hh, int mm, const char *title)
{
    for (int i = 0; i < n_tasks; i++)
        if (tasks[i].year   == y  && tasks[i].month  == m  &&
            tasks[i].day    == d  && tasks[i].hour   == hh &&
            tasks[i].minute == mm && strcmp(tasks[i].title, title) == 0)
            return i;
    return -1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 중복 검사 (반복 포함)
 *
 * (y,m,d,hh,mm,title) 조합이 이미 어떤 task (exclude_idx 제외) 의
 * 발생일에 해당하는지 확인
 * ═══════════════════════════════════════════════════════════════════════ */
static int is_duplicate(int y, int m, int d, int hh, int mm,
                        const char *title, int exclude_idx)
{
    for (int i = 0; i < n_tasks; i++) {
        if (i == exclude_idx) continue;
        if (!task_occurs_on(i, y, m, d)) continue;
        if (tasks[i].hour   == hh  &&
            tasks[i].minute == mm  &&
            strcmp(tasks[i].title, title) == 0)
            return 1;
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * task_upsert
 * ═══════════════════════════════════════════════════════════════════════ */
int task_upsert(int y, int m, int d, int hh, int mm, const char *title)
{
    int idx = task_find_by_title(y, m, d, title);

    if (idx >= 0) {
        /* 이미 존재: 시각이 동일하면 -2(중복), 다르면 갱신 */
        if (tasks[idx].hour == hh && tasks[idx].minute == mm)
            return -2;
        /* 새 시각으로 바꿨을 때 다른 task와 충돌하는지 검사 */
        if (is_duplicate(y, m, d, hh, mm, title, idx))
            return -2;
        tasks[idx].hour   = hh;
        tasks[idx].minute = mm;
        task_sort();
        return 1;   /* 수정 */
    }

    /* 신규 추가: 중복 검사 */
    if (is_duplicate(y, m, d, hh, mm, title, -1))
        return -2;
    if (n_tasks >= MAX_TASKS) return -1;

    Task *t   = &tasks[n_tasks++];
    t->year   = y;  t->month       = m;   t->day         = d;
    t->hour   = hh; t->minute      = mm;
    t->repeat_days = 0;
    snprintf(t->title, TITLE_LEN, "%s", title);
    t->desc[0] = '\0';

    task_sort();
    return 0;   /* 추가 */
}

/* ═══════════════════════════════════════════════════════════════════════
 * 삭제
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
    int removed = 0;
    for (int i = n_tasks - 1; i >= 0; i--) {
        if (tasks[i].year  == y && tasks[i].month == m &&
            tasks[i].day   == d && strcmp(tasks[i].title, title) == 0) {
            task_del(i);
            removed++;
        }
    }
    return removed;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 반복 설정
 * ═══════════════════════════════════════════════════════════════════════ */
int task_set_repeat(int idx, int n)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    tasks[idx].repeat_days = n;
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 설명
 * ═══════════════════════════════════════════════════════════════════════ */
int task_set_desc(int idx, const char *desc)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    snprintf(tasks[idx].desc, DESC_LEN, "%s", desc);
    return 1;
}

int task_clear_desc(int idx)
{
    if (idx < 0 || idx >= n_tasks) return 0;
    tasks[idx].desc[0] = '\0';
    return 1;
}
