#ifndef UTIL_H
#define UTIL_H

/* ── ANSI 색상 코드 ───────────────────────────────────────────────────── */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"

/* ── UTF-8 트리 기호 ──────────────────────────────────────────────────── */
#define T_MID  "├── "
#define T_END  "└── "
#define T_CON  "│   "
#define T_SPC  "    "

/* ── 한국어 상수 ──────────────────────────────────────────────────────── */
extern const char *wday_kr[];
extern const char *month_kr[];

/* ── 함수 선언 ────────────────────────────────────────────────────────── */
void hr_line(int width);
int  days_in_month(int year, int month);

/*
 * Zeller's congruence
 * 반환값: 0=일, 1=월, 2=화, 3=수, 4=목, 5=금, 6=토
 */
int  day_of_week(int year, int month, int day);

/* 오늘 날짜 취득 */
void today(int *year, int *month, int *day);

/* 날짜 유효성 검사 (1 = 유효, 0 = 무효) */
int  date_valid(int year, int month, int day);
int  yearmonth_valid(int year, int month);

#endif /* UTIL_H */
