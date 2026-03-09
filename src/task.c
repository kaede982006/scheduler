#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "task.h"

/* ── 전역 저장소 ──────────────────────────────────────────────────────── */
Task tasks[MAX_TASKS];
int  n_tasks = 0;

static char data_file[512];

/* ── 데이터 경로 초기화 ───────────────────────────────────────────────── */
void task_init_path(void)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";

    char buf[480];
    snprintf(buf, sizeof buf, "%s/.local",               home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share",         home); mkdir(buf, 0755);
    snprintf(buf, sizeof buf, "%s/.local/share/scheduler", home); mkdir(buf, 0755);
    snprintf(data_file, sizeof data_file,
             "%s/.local/share/scheduler/tasks.csv", home);
}

/* ── 정렬 비교 함수 ───────────────────────────────────────────────────── */
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

/* ── 로드 ─────────────────────────────────────────────────────────────── */
void task_load(void)
{
    FILE *f = fopen(data_file, "r");
    if (!f) return;

    char line[1024];
    n_tasks = 0;

    while (fgets(line, sizeof line, f) && n_tasks < MAX_TASKS) {
        Task *t = &tasks[n_tasks];
        if (sscanf(line, "%d,%d,%d,%d,%d,",
                   &t->year, &t->month, &t->day,
                   &t->hour, &t->minute) != 5)
            continue;

        /* 5번째 쉼표 이후 = 제목 */
        char *p = line;
        int commas = 0;
        while (*p && commas < 5) { if (*p++ == ',') commas++; }
        size_t len = strlen(p);
        while (len && (p[len-1] == '\n' || p[len-1] == '\r')) p[--len] = '\0';

        snprintf(t->title, TITLE_LEN, "%s", p);
        n_tasks++;
    }
    fclose(f);
    task_sort();
}

/* ── 저장 ─────────────────────────────────────────────────────────────── */
void task_save(void)
{
    task_sort();
    FILE *f = fopen(data_file, "w");
    if (!f) { perror("task_save: fopen"); return; }

    for (int i = 0; i < n_tasks; i++)
        fprintf(f, "%04d,%02d,%02d,%02d,%02d,%s\n",
                tasks[i].year,  tasks[i].month, tasks[i].day,
                tasks[i].hour,  tasks[i].minute, tasks[i].title);

    fclose(f);
}

/* ── 쿼리 ─────────────────────────────────────────────────────────────── */
int task_query_day(int y, int m, int d, int out[], int cap)
{
    int cnt = 0;
    for (int i = 0; i < n_tasks && cnt < cap; i++)
        if (tasks[i].year == y && tasks[i].month == m && tasks[i].day == d)
            out[cnt++] = i;
    return cnt;
}

int task_query_hour(int y, int m, int d, int h, int out[], int cap)
{
    int cnt = 0;
    for (int i = 0; i < n_tasks && cnt < cap; i++)
        if (tasks[i].year == y && tasks[i].month == m &&
            tasks[i].day  == d && tasks[i].hour  == h)
            out[cnt++] = i;
    return cnt;
}

/* ── CRUD ─────────────────────────────────────────────────────────────── */
int task_add(int y, int m, int d, int hh, int mm, const char *title)
{
    if (n_tasks >= MAX_TASKS) return -1;

    Task *t  = &tasks[n_tasks++];
    t->year  = y;  t->month  = m;  t->day    = d;
    t->hour  = hh; t->minute = mm;
    snprintf(t->title, TITLE_LEN, "%s", title);

    task_sort();
    return 0;
}

int task_del(int id)
{
    if (id < 0 || id >= n_tasks) return 0;
    memmove(&tasks[id], &tasks[id + 1],
            (size_t)(n_tasks - id - 1) * sizeof(Task));
    n_tasks--;
    return 1;
}

int task_edit(int id, int hh, int mm, const char *title)
{
    if (id < 0 || id >= n_tasks) return 0;
    tasks[id].hour   = hh;
    tasks[id].minute = mm;
    snprintf(tasks[id].title, TITLE_LEN, "%s", title);
    task_sort();
    return 1;
}
