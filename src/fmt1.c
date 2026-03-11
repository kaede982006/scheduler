#include <stdio.h>
#include <string.h>
#include "util.h"
#include "task.h"
#include "fmt1.h"

/* ─ 해당 날짜의 루트 노드 수 계산 ──────────────────────────────────── */
static int count_roots(const int *idx, int cnt)
{
    /* 중복 없이 루트 segment 개수 반환 */
    char roots[MAX_TASKS][TITLE_LEN];
    int  nr = 0;
    for (int i = 0; i < cnt; i++) {
        char root[TITLE_LEN];
        path_basename(tasks[idx[i]].title, root, TITLE_LEN);
        /* root segment 는 '/' 가 없는 부분 → path_basename 이 아니라 첫 segment */
        const char *title = tasks[idx[i]].title;
        const char *slash = strchr(title, '/');
        if (slash) {
            int len = (int)(slash - title);
            if (len >= TITLE_LEN) len = TITLE_LEN - 1;
            memcpy(root, title, (size_t)len);
            root[len] = '\0';
        } else {
            snprintf(root, TITLE_LEN, "%s", title);
        }
        int dup = 0;
        for (int j = 0; j < nr; j++)
            if (strcmp(roots[j], root) == 0) { dup = 1; break; }
        if (!dup && nr < MAX_TASKS)
            snprintf(roots[nr++], TITLE_LEN, "%s", root);
    }
    return nr;
}

/* ── 1형식 / 텍스트 라인 ─────────────────────────────────────────────── */
void fmt1_text_lines(int y, int m)
{
    printf(C_BOLD C_CYAN
           "\n◆ [1형식 / 텍스트 라인]  %04d년 %s\n" C_RESET,
           y, month_kr[m]);
    hr_line(52);

    int dm = days_in_month(y, m), found = 0;

    for (int d = 1; d <= dm; d++) {
        int idx[256], cnt = task_query_day(y, m, d, idx, 256);
        if (cnt == 0) continue;
        found = 1;

        int         dow = day_of_week(y, m, d);
        const char *dc  = (dow == 0 ? C_RED : (dow == 6 ? C_BLUE : C_WHITE));
        int         nr  = count_roots(idx, cnt);

        printf(C_BOLD "%s%04d-%02d-%02d (%s)" C_RESET
               C_DIM " [%d건]" C_RESET "\n",
               dc, y, m, d, wday_kr[dow], nr);

        task_render_tree(idx, cnt, "", "  ");
    }
    if (!found)
        printf(C_DIM "  (이번 달 등록된 일정이 없습니다)\n" C_RESET);
    printf("\n");
}

/* ── 1형식 / 트리 ────────────────────────────────────────────────────── */
void fmt1_tree(int y, int m)
{
    printf(C_BOLD C_CYAN
           "◆ [1형식 / 트리]  %04d년 %s\n" C_RESET, y, month_kr[m]);

    int dm = days_in_month(y, m);
    int days_with[32], nd = 0;
    for (int d = 1; d <= dm; d++) {
        int tmp[1];
        if (task_query_day(y, m, d, tmp, 1) > 0)
            days_with[nd++] = d;
    }

    if (nd == 0) {
        printf(C_DIM "  (이번 달 등록된 일정이 없습니다)\n\n" C_RESET);
        return;
    }

    printf(C_BOLD "%04d-%02d\n" C_RESET, y, m);

    for (int di = 0; di < nd; di++) {
        int d   = days_with[di];
        int idx[256], cnt = task_query_day(y, m, d, idx, 256);
        int dow = day_of_week(y, m, d);
        int nr  = count_roots(idx, cnt);

        const char *hlast   = (di == nd - 1) ? T_END : T_MID;
        const char *hindent = (di == nd - 1) ? T_SPC : T_CON;
        const char *dc      = (dow == 0 ? C_RED : (dow == 6 ? C_BLUE : C_WHITE));

        printf("%s" C_BOLD "%s%02d일 (%s)" C_RESET
               C_DIM " [%d건]" C_RESET "\n",
               hlast, dc, d, wday_kr[dow], nr);

        task_render_tree(idx, cnt, "", hindent);
    }
    printf("\n");
}

void fmt1_render(int y, int m)
{
    fmt1_text_lines(y, m);
    fmt1_tree(y, m);
}
