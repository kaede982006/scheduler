#include <stdio.h>
#include "util.h"
#include "task.h"
#include "fmt2.h"

static void repeat_marker(Task *t, char *buf, int cap)
{
    if (t->repeat_days > 0)
        snprintf(buf, cap, C_DIM " [↻%d일]" C_RESET, t->repeat_days);
    else
        buf[0] = '\0';
}

void fmt2_text_lines(int y, int m, int d)
{
    int         dow = day_of_week(y, m, d);
    const char *dc  = (dow==0 ? C_RED : (dow==6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN
           "\n◆ [2형식 / 텍스트 라인]  %04d년 %s %d일 %s(%s)" C_RESET "\n",
           y, month_kr[m], d, dc, wday_kr[dow]);
    hr_line(52);

    int idx[64];
    int total = task_query_day(y, m, d, idx, 64);

    if (total == 0) {
        printf(C_DIM "  (이 날 등록된 일정이 없습니다)\n" C_RESET);
    } else {
        int cur_h = -1;
        for (int i = 0; i < total; i++) {
            Task *t = &tasks[idx[i]];
            if (t->hour != cur_h) {
                cur_h = t->hour;
                printf(C_YELLOW "  %02d:00 대\n" C_RESET, cur_h);
            }
            char rm[48];
            repeat_marker(t, rm, sizeof rm);
            printf("    " C_GREEN "[%02d:%02d]" C_RESET "  %s%s\n",
                   t->hour, t->minute, t->title, rm);
        }
    }
    printf("\n");
}

void fmt2_tree(int y, int m, int d)
{
    int         dow = day_of_week(y, m, d);
    const char *dc  = (dow==0 ? C_RED : (dow==6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN
           "◆ [2형식 / 트리]  %04d년 %s %d일 %s(%s)" C_RESET "\n",
           y, month_kr[m], d, dc, wday_kr[dow]);

    int idx_all[64];
    int total = task_query_day(y, m, d, idx_all, 64);

    printf(C_BOLD "%s%04d-%02d-%02d\n" C_RESET, dc, y, m, d);

    if (total == 0) {
        printf(T_END C_DIM "(일정 없음)\n\n" C_RESET);
        return;
    }

    /* 시간대 그룹 수집 */
    int seen[24] = {0}, hours[24], nh = 0;
    for (int i = 0; i < total; i++) {
        int h = tasks[idx_all[i]].hour;
        if (h >= 0 && h < 24 && !seen[h]) { seen[h]=1; hours[nh++]=h; }
    }

    for (int hi = 0; hi < nh; hi++) {
        int h    = hours[hi];
        int hidx[32];
        int hcnt = task_query_hour(y, m, d, h, hidx, 32);

        const char *hlast  = (hi == nh-1) ? T_END : T_MID;
        const char *hindent= (hi == nh-1) ? T_SPC : T_CON;

        printf("%s" C_YELLOW "%02d:00" C_RESET
               C_DIM " [%d건]" C_RESET "\n", hlast, h, hcnt);

        for (int i = 0; i < hcnt; i++) {
            Task       *t   = &tasks[hidx[i]];
            const char *pfx = (i == hcnt-1) ? T_END : T_MID;
            char        rm[48];
            repeat_marker(t, rm, sizeof rm);
            printf("%s%s" C_GREEN "[%02d:%02d]" C_RESET "  %s%s\n",
                   hindent, pfx, t->hour, t->minute, t->title, rm);
        }
    }
    printf("\n");
}

void fmt2_render(int y, int m, int d)
{
    fmt2_text_lines(y, m, d);
    fmt2_tree(y, m, d);
}
