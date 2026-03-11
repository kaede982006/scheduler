#include <stdio.h>
#include "util.h"
#include "task.h"
#include "fmt2.h"

/* ── 2형식 / 텍스트 라인 ─────────────────────────────────────────────── */
void fmt2_text_lines(int y, int m, int d)
{
    int         dow = day_of_week(y, m, d);
    const char *dc  = (dow == 0 ? C_RED : (dow == 6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN
           "\n◆ [2형식 / 텍스트 라인]  %04d년 %s %d일 %s(%s)" C_RESET "\n",
           y, month_kr[m], d, dc, wday_kr[dow]);
    hr_line(52);

    int idx[256], total = task_query_day(y, m, d, idx, 256);

    if (total == 0)
        printf(C_DIM "  (이 날 등록된 일정이 없습니다)\n" C_RESET);
    else
        task_render_tree(idx, total, "", "  ");
    printf("\n");
}

/* ── 2형식 / 트리 ────────────────────────────────────────────────────── */
void fmt2_tree(int y, int m, int d)
{
    int         dow = day_of_week(y, m, d);
    const char *dc  = (dow == 0 ? C_RED : (dow == 6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN
           "◆ [2형식 / 트리]  %04d년 %s %d일 %s(%s)" C_RESET "\n",
           y, month_kr[m], d, dc, wday_kr[dow]);

    int idx[256], total = task_query_day(y, m, d, idx, 256);

    printf(C_BOLD "%s%04d-%02d-%02d\n" C_RESET, dc, y, m, d);

    if (total == 0)
        printf(T_END C_DIM "(일정 없음)\n\n" C_RESET);
    else {
        task_render_tree(idx, total, "", "");
        printf("\n");
    }
}

void fmt2_render(int y, int m, int d)
{
    fmt2_text_lines(y, m, d);
    fmt2_tree(y, m, d);
}
