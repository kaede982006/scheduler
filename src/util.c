#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "util.h"

const char *wday_kr[]  = {"일","월","화","수","목","금","토"};
const char *month_kr[] = {
    "","1월","2월","3월","4월","5월","6월",
    "7월","8월","9월","10월","11월","12월"
};

void hr_line(int width)
{
    for (int i = 0; i < width; i++) printf("─");
    printf("\n");
}

int days_in_month(int y, int m)
{
    static const int d[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return d[m];
}

int day_of_week(int y, int m, int d)
{
    if (m < 3) { m += 12; y--; }
    int k = y % 100, j = y / 100;
    int h = (d + (13*(m+1))/5 + k + k/4 + j/4 - 2*j) % 7;
    h = ((h % 7) + 7) % 7;
    return (h + 6) % 7;
}

void today(int *year, int *month, int *day)
{
    time_t     now = time(NULL);
    struct tm *lt  = localtime(&now);
    *year  = lt->tm_year + 1900;
    *month = lt->tm_mon  + 1;
    *day   = lt->tm_mday;
}

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

/*
 * Proleptic Gregorian → Julian Day Number (simplified)
 * 같은 달력 체계 내에서 두 날짜의 차이(일수) 계산에만 사용
 * 공식: Hatcher (1984) variant
 */
long date_to_days(int y, int m, int d)
{
    if (m <= 2) { m += 12; y -= 1; }
    return 365L*y + y/4 - y/100 + y/400
           + (153*m - 457)/5 + d - 306;
}
