#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "util.h"

/* ── 한국어 상수 정의 ─────────────────────────────────────────────────── */
const char *wday_kr[]  = {"일","월","화","수","목","금","토"};
const char *month_kr[] = {
    "","1월","2월","3월","4월","5월","6월",
    "7월","8월","9월","10월","11월","12월"
};

/* ── 수평선 출력 ──────────────────────────────────────────────────────── */
void hr_line(int width)
{
    for (int i = 0; i < width; i++) printf("─");
    printf("\n");
}

/* ── 월의 일 수 ───────────────────────────────────────────────────────── */
int days_in_month(int y, int m)
{
    static const int d[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        return 29;
    return d[m];
}

/* ── Zeller's Congruence ──────────────────────────────────────────────── */
int day_of_week(int y, int m, int d)
{
    if (m < 3) { m += 12; y--; }
    int k = y % 100;
    int j = y / 100;
    int h = (d + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    h = ((h % 7) + 7) % 7;   /* 음수 방지 */
    return (h + 6) % 7;       /* 0=일 … 6=토 */
}

/* ── 오늘 날짜 ────────────────────────────────────────────────────────── */
void today(int *year, int *month, int *day)
{
    time_t     now = time(NULL);
    struct tm *lt  = localtime(&now);
    *year  = lt->tm_year + 1900;
    *month = lt->tm_mon  + 1;
    *day   = lt->tm_mday;
}

/* ── 날짜 유효성 ──────────────────────────────────────────────────────── */
int date_valid(int y, int m, int d)
{
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > days_in_month(y, m)) return 0;
    return 1;
}

int yearmonth_valid(int y, int m)
{
    return (m >= 1 && m <= 12 && y >= 1);
}
