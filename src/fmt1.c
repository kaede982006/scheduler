#include <stdio.h>
#include "util.h"
#include "task.h"
#include "fmt1.h"

/* ── 1형식: 텍스트 라인 ──────────────────────────────────────────────── */
void fmt1_text_lines(int y, int m)
{
    printf(C_BOLD C_CYAN
           "\n◆ [1형식 / 텍스트 라인]  %04d년 %s\n" C_RESET, y, month_kr[m]);
    hr_line(52);

    int dm    = days_in_month(y, m);
    int found = 0;

    for (int d = 1; d <= dm; d++) {
        int idx[64];
        int cnt = task_query_day(y, m, d, idx, 64);
        if (cnt == 0) continue;
        found = 1;

        int         dow = day_of_week(y, m, d);
        const char *dc  = (dow == 0 ? C_RED  :
                          (dow == 6 ? C_BLUE : C_WHITE));

        printf(C_BOLD "%s%04d-%02d-%02d (%s)" C_RESET "\n",
               dc, y, m, d, wday_kr[dow]);

        for (int i = 0; i < cnt; i++) {
            Task *t = &tasks[idx[i]];
            printf("  " C_GREEN "[%02d:%02d]" C_RESET "  %s\n",
                   t->hour, t->minute, t->title);
        }
    }
    if (!found)
        printf(C_DIM "  (이번 달 등록된 일정이 없습니다)\n" C_RESET);

    printf("\n");
}

/* ── 1형식: 트리 ─────────────────────────────────────────────────────── */
void fmt1_tree(int y, int m)
{
    printf(C_BOLD C_CYAN
           "◆ [1형식 / 트리]  %04d년 %s\n" C_RESET, y, month_kr[m]);

    int dm = days_in_month(y, m);

    /* 일정이 있는 날만 수집 */
    int days_with[32];
    int nd = 0;
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
        int idx[64];
        int cnt = task_query_day(y, m, d, idx, 64);
        int dow = day_of_week(y, m, d);

        const char *hlast  = (di == nd - 1) ? T_END : T_MID;
        const char *hindent= (di == nd - 1) ? T_SPC : T_CON;
        const char *dc     = (dow == 0 ? C_RED  :
                             (dow == 6 ? C_BLUE : C_WHITE));

        printf("%s" C_BOLD "%s%02d일 (%s)" C_RESET
               C_DIM " [%d건]" C_RESET "\n",
               hlast, dc, d, wday_kr[dow], cnt);

        for (int i = 0; i < cnt; i++) {
            Task       *t   = &tasks[idx[i]];
            const char *pfx = (i == cnt - 1) ? T_END : T_MID;
            printf("%s%s" C_GREEN "[%02d:%02d]" C_RESET "  %s\n",
                   hindent, pfx, t->hour, t->minute, t->title);
        }
    }
    printf("\n");
}

/* ── 래퍼 ─────────────────────────────────────────────────────────────── */
void fmt1_render(int y, int m)
{
    fmt1_text_lines(y, m);
    fmt1_tree(y, m);
}
