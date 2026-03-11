#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "task.h"
#include "fmt1.h"
#include "fmt2.h"

/* ═══════════════════════════════════════════════════════════════════════
 * 사용법
 * ═══════════════════════════════════════════════════════════════════════ */
static void usage(const char *prog)
{
    printf(C_BOLD "\n  %s" C_RESET " — CLI 스케줄러\n\n", prog);

    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "(인자 없음) / -M [YYYY-MM]",
           "1형식: 월간 달력 뷰");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-D [YYYY-MM-DD]",
           "2형식: 일간 시간 뷰");
    printf("\n");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-t YYYY-MM-DD HH:MM <제목> [HH:MM <제목> ...]",
           "일정 추가/수정 (upsert)");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-r YYYY-MM-DD HH:MM <제목> [HH:MM <제목> ...]",
           "일정 삭제");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-c YYYY-MM-DD HH:MM <제목> <N>",
           "N일마다 반복 설정 (0=해제)");
    printf("\n");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-p YYYY-MM-DD HH:MM <제목>",
           "설명 출력");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-i YYYY-MM-DD HH:MM <제목> <설명>",
           "설명 추가/수정");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-d YYYY-MM-DD HH:MM <제목>",
           "설명 삭제");
    printf("\n");
    printf("  " C_YELLOW "%-48s" C_RESET "%s\n",
           "-h / --help",
           "도움말");
    printf("\n");
    printf("  날짜 생략 시 오늘 날짜를 사용합니다.\n");
    printf("  각 옵션은 독립적으로 수행됩니다.\n\n");
}

/* ═══════════════════════════════════════════════════════════════════════
 * 파싱 헬퍼
 * ═══════════════════════════════════════════════════════════════════════ */
static int parse_date(const char *s, int *y, int *m, int *d)
{
    if (sscanf(s, "%d-%d-%d", y, m, d) != 3) return 0;
    return date_valid(*y, *m, *d);
}

static int parse_yearmonth(const char *s, int *y, int *m)
{
    if (sscanf(s, "%d-%d", y, m) != 2) return 0;
    return yearmonth_valid(*y, *m);
}

static int parse_time(const char *s, int *h, int *min)
{
    return (sscanf(s, "%d:%d", h, min) == 2 &&
            *h >= 0 && *h <= 23 && *min >= 0 && *min <= 59);
}

/* 문자열이 HH:MM 패턴인지 빠른 판별 */
static int looks_like_time(const char *s)
{
    int h, m;
    return (sscanf(s, "%d:%d", &h, &m) == 2 &&
            h >= 0 && h <= 23 && m >= 0 && m <= 59);
}

/* ─ 인자 부족 오류 출력 ─ */
static void err_args(const char *opt, const char *fmt)
{
    fprintf(stderr, C_RED "오류: %s %s\n" C_RESET, opt, fmt);
}

/* ─ 날짜 파싱 오류 출력 ─ */
static void err_date(const char *opt, const char *raw)
{
    fprintf(stderr,
        C_RED "오류: %s — 유효하지 않은 날짜 '%s'\n" C_RESET, opt, raw);
}

/* ─ 시각 파싱 오류 출력 ─ */
static void err_time(const char *opt, const char *raw)
{
    fprintf(stderr,
        C_RED "오류: %s — 유효하지 않은 시각 '%s' (HH:MM 필요)\n"
        C_RESET, opt, raw);
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -M
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_M(int argc, char *argv[], int i)
{
    int cy, cm, cd;
    today(&cy, &cm, &cd);
    int y = cy, m = cm;

    if (i < argc && argv[i][0] != '-') {
        if (!parse_yearmonth(argv[i], &y, &m)) {
            err_date("-M", argv[i]); return 1;
        }
    }
    fmt1_render(y, m);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -D
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_D(int argc, char *argv[], int i)
{
    int cy, cm, cd;
    today(&cy, &cm, &cd);
    int y = cy, m = cm, d = cd;

    if (i < argc && argv[i][0] != '-') {
        if (!parse_date(argv[i], &y, &m, &d)) {
            err_date("-D", argv[i]); return 1;
        }
    }
    fmt2_render(y, m, d);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -t  YYYY-MM-DD  HH:MM <제목> [HH:MM <제목> ...]
 *
 * 동작 (upsert):
 *   - (날짜, 제목) 이 이미 존재 → 시각 갱신
 *   - 없으면 신규 추가
 *   - (날짜, 시각, 제목) 완전 중복 → 오류
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_t(int argc, char *argv[], int i)
{
    if (i >= argc) { err_args("-t", "YYYY-MM-DD HH:MM <제목> [...]"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-t", argv[i]); return 1; }
    i++;

    if (i >= argc || looks_like_time(argv[i]) == 0) {
        err_args("-t", "YYYY-MM-DD HH:MM <제목> [...]"); return 1;
    }

    int changed = 0, errors = 0;

    while (i < argc && argv[i][0] != '-') {
        /* HH:MM */
        int hh, mm;
        if (!parse_time(argv[i], &hh, &mm)) { err_time("-t", argv[i]); return 1; }
        i++;

        /* 제목 */
        if (i >= argc) { err_args("-t", "HH:MM 뒤에 제목이 필요합니다."); return 1; }
        const char *title = argv[i++];

        int rc = task_upsert(y, m, d, hh, mm, title);
        switch (rc) {
        case 0:
            printf(C_GREEN "✔ 추가됨: %04d-%02d-%02d [%02d:%02d] %s\n"
                   C_RESET, y, m, d, hh, mm, title);
            changed++;
            break;
        case 1:
            printf(C_CYAN "✎ 수정됨: %04d-%02d-%02d [%02d:%02d] %s\n"
                   C_RESET, y, m, d, hh, mm, title);
            changed++;
            break;
        case -2:
            fprintf(stderr,
                C_YELLOW "  이미 존재: %04d-%02d-%02d [%02d:%02d] %s\n"
                C_RESET, y, m, d, hh, mm, title);
            errors++;
            break;
        case -1:
        default:
            fprintf(stderr,
                C_RED "오류: 최대 일정 수(%d) 초과.\n" C_RESET, MAX_TASKS);
            return 1;
        }
    }

    if (changed > 0) {
        task_save();
        printf(C_DIM "  → %d건 저장됨\n" C_RESET, changed);
    }
    return (errors > 0 && changed == 0) ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -r  YYYY-MM-DD  <제목> [<제목> ...]
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_r(int argc, char *argv[], int i)
{
    if (i >= argc) { err_args("-r", "YYYY-MM-DD HH:MM <제목> [HH:MM <제목> ...]"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-r", argv[i]); return 1; }
    i++;

    if (i >= argc) {
        fprintf(stderr, C_RED "오류: 삭제할 HH:MM <제목> 을 입력하세요.\n" C_RESET);
        return 1;
    }

    int total_removed = 0;
    while (i < argc && argv[i][0] != '-') {
        /* HH:MM */
        int hh, mm;
        if (!parse_time(argv[i], &hh, &mm)) { err_time("-r", argv[i]); return 1; }
        i++;

        /* 제목 */
        if (i >= argc) { err_args("-r", "HH:MM 뒤에 제목이 필요합니다."); return 1; }
        const char *title = argv[i++];

        int idx = task_find_exact(y, m, d, hh, mm, title);
        if (idx < 0) {
            printf(C_YELLOW "  없음: %04d-%02d-%02d [%02d:%02d] '%s'\n"
                   C_RESET, y, m, d, hh, mm, title);
        } else {
            task_del(idx);
            printf(C_RED "✖ 삭제됨: %04d-%02d-%02d [%02d:%02d] %s\n"
                   C_RESET, y, m, d, hh, mm, title);
            total_removed++;
        }
    }

    if (total_removed > 0) {
        task_save();
        printf(C_DIM "  → 총 %d건 삭제됨\n" C_RESET, total_removed);
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -c  YYYY-MM-DD  HH:MM  <제목>  <N>
 *
 * N > 0: N일마다 반복 설정
 * N = 0: 반복 해제
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_c(int argc, char *argv[], int i)
{
    if (i + 3 >= argc) {
        err_args("-c", "YYYY-MM-DD HH:MM <제목> <N>"); return 1;
    }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-c", argv[i]); return 1; }
    i++;

    int hh, mm;
    if (!parse_time(argv[i], &hh, &mm)) { err_time("-c", argv[i]); return 1; }
    i++;

    const char *title = argv[i++];
    int n = -1;
    if (sscanf(argv[i], "%d", &n) != 1 || n < 0) {
        fprintf(stderr,
            C_RED "오류: -c 반복 횟수 N 은 0 이상의 정수여야 합니다.\n"
            C_RESET);
        return 1;
    }

    int idx = task_find_exact(y, m, d, hh, mm, title);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' [%02d:%02d] — %04d-%02d-%02d 에 해당 일정 없음\n"
            C_RESET, title, hh, mm, y, m, d);
        return 1;
    }

    task_set_repeat(idx, n);
    task_save();

    if (n == 0)
        printf(C_CYAN "↻ 반복 해제: %04d-%02d-%02d [%02d:%02d] %s\n"
               C_RESET, y, m, d, hh, mm, title);
    else
        printf(C_CYAN "↻ 반복 설정: %04d-%02d-%02d [%02d:%02d] %s — %d일마다\n"
               C_RESET, y, m, d, hh, mm, title, n);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -p  YYYY-MM-DD  HH:MM  <제목>
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_p(int argc, char *argv[], int i)
{
    if (i + 2 >= argc) { err_args("-p", "YYYY-MM-DD HH:MM <제목>"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-p", argv[i]); return 1; }
    i++;

    int hh, mm;
    if (!parse_time(argv[i], &hh, &mm)) { err_time("-p", argv[i]); return 1; }
    i++;

    const char *title = argv[i];

    int idx = task_find_exact(y, m, d, hh, mm, title);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' [%02d:%02d] — %04d-%02d-%02d 에 해당 일정 없음\n"
            C_RESET, title, hh, mm, y, m, d);
        return 1;
    }

    if (tasks[idx].desc[0] == '\0') {
        printf(C_DIM "  (설명 없음)\n" C_RESET);
        return 0;
    }

    printf(C_BOLD C_CYAN
           "◆ 설명: %04d-%02d-%02d [%02d:%02d] %s\n" C_RESET,
           y, m, d, hh, mm, title);
    hr_line(52);
    printf("  %s\n\n", tasks[idx].desc);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -i  YYYY-MM-DD  HH:MM  <제목>  <설명>
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_i(int argc, char *argv[], int i)
{
    if (i + 3 >= argc) {
        err_args("-i", "YYYY-MM-DD HH:MM <제목> <설명>"); return 1;
    }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-i", argv[i]); return 1; }
    i++;

    int hh, mm;
    if (!parse_time(argv[i], &hh, &mm)) { err_time("-i", argv[i]); return 1; }
    i++;

    const char *title = argv[i++];
    const char *desc  = argv[i];

    int idx = task_find_exact(y, m, d, hh, mm, title);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' [%02d:%02d] — %04d-%02d-%02d 에 해당 일정 없음\n"
            C_RESET, title, hh, mm, y, m, d);
        return 1;
    }

    const char *action = (tasks[idx].desc[0] != '\0') ? "수정" : "추가";
    task_set_desc(idx, desc);
    task_save();
    printf(C_GREEN "✔ 설명 %s됨: %04d-%02d-%02d [%02d:%02d] %s\n"
           C_RESET, action, y, m, d, hh, mm, title);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -d  YYYY-MM-DD  HH:MM  <제목>   (설명 삭제)
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_d(int argc, char *argv[], int i)
{
    if (i + 2 >= argc) {
        err_args("-d", "YYYY-MM-DD HH:MM <제목>"); return 1;
    }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-d", argv[i]); return 1; }
    i++;

    int hh, mm;
    if (!parse_time(argv[i], &hh, &mm)) { err_time("-d", argv[i]); return 1; }
    i++;

    const char *title = argv[i];

    int idx = task_find_exact(y, m, d, hh, mm, title);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' [%02d:%02d] — %04d-%02d-%02d 에 해당 일정 없음\n"
            C_RESET, title, hh, mm, y, m, d);
        return 1;
    }

    if (tasks[idx].desc[0] == '\0') {
        printf(C_DIM "  (이미 설명이 없습니다)\n" C_RESET);
        return 0;
    }

    task_clear_desc(idx);
    task_save();
    printf(C_RED "✖ 설명 삭제됨: %04d-%02d-%02d [%02d:%02d] %s\n"
           C_RESET, y, m, d, hh, mm, title);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    task_init_path();
    task_load();

    /* 인자 없음 → 이번 달 1형식 뷰 */
    if (argc < 2) {
        int cy, cm, cd;
        today(&cy, &cm, &cd);
        fmt1_render(cy, cm);
        return 0;
    }

    const char *opt  = argv[1];
    int         rest = 2;

    if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) {
        usage(argv[0]); return 0;
    }
    if (!strcmp(opt, "-M")) return handle_M(argc, argv, rest);
    if (!strcmp(opt, "-D")) return handle_D(argc, argv, rest);
    if (!strcmp(opt, "-t")) return handle_t(argc, argv, rest);
    if (!strcmp(opt, "-r")) return handle_r(argc, argv, rest);
    if (!strcmp(opt, "-c")) return handle_c(argc, argv, rest);
    if (!strcmp(opt, "-p")) return handle_p(argc, argv, rest);
    if (!strcmp(opt, "-i")) return handle_i(argc, argv, rest);
    if (!strcmp(opt, "-d")) return handle_d(argc, argv, rest);

    fprintf(stderr, C_RED "오류: 알 수 없는 옵션 '%s'\n" C_RESET, opt);
    usage(argv[0]);
    return 1;
}
