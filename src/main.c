#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "task.h"
#include "fmt1.h"
#include "fmt2.h"
#include "edit.h"

/* ── 모드 열거 ────────────────────────────────────────────────────────── */
typedef enum {
    MODE_NONE = 0,
    MODE_FMT1,   /* -d: 1형식 읽기 */
    MODE_FMT2,   /* -u: 2형식 읽기 */
    MODE_EDIT    /* -m: 2형식 편집 (단독 사용) */
} Mode;

/* ── 사용법 ───────────────────────────────────────────────────────────── */
static void usage(const char *prog)
{
    printf(C_BOLD "\n  %s" C_RESET " — CLI 스케줄러\n\n", prog);
    printf("  " C_YELLOW "%-32s" C_RESET "%s\n",
           "-d [YYYY-MM]",
           "1형식: 월간 달력 뷰 (읽기)");
    printf("  " C_YELLOW "%-32s" C_RESET "%s\n",
           "-u [YYYY-MM-DD]",
           "2형식: 일간 시간 뷰 (읽기)");
    printf("  " C_YELLOW "%-32s" C_RESET "%s\n",
           "-m [YYYY-MM-DD]",
           "2형식: 편집 모드  ※ 단독 옵션");
    printf("\n");
    printf("  날짜를 생략하면 오늘 날짜를 사용합니다.\n");
    printf("  -m 은 다른 옵션과 함께 사용할 수 없습니다.\n\n");
}

/* ── main ─────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    /* 데이터 경로 초기화 및 로드 */
    task_init_path();
    task_load();

    /* 오늘 날짜 기본값 */
    int cy, cm, cd;
    today(&cy, &cm, &cd);

    /* 옵션 파싱 */
    Mode mode    = MODE_NONE;
    int  d_year  = cy, d_month = cm;           /* -d 날짜 */
    int  u_year  = cy, u_month = cm, u_day = cd; /* -u / -m 날짜 */

    if (argc < 2) { usage(argv[0]); return 0; }

    for (int i = 1; i < argc; i++) {

        /* ── -d ───────────────────────────────────────────────────── */
        if (!strcmp(argv[i], "-d")) {
            if (mode == MODE_EDIT) {
                fprintf(stderr,
                    C_RED "오류: -m 은 단독 사용 옵션입니다. "
                    "다른 옵션과 함께 사용할 수 없습니다.\n" C_RESET);
                return 1;
            }
            if (mode == MODE_FMT2) {
                fprintf(stderr,
                    C_RED "오류: -d 와 -u 는 동시에 사용할 수 없습니다.\n"
                    C_RESET);
                return 1;
            }
            mode = MODE_FMT1;

            if (i + 1 < argc && argv[i+1][0] != '-') {
                i++;
                if (sscanf(argv[i], "%d-%d", &d_year, &d_month) != 2) {
                    fprintf(stderr,
                        C_RED "오류: -d 날짜 형식은 YYYY-MM 입니다.\n" C_RESET);
                    return 1;
                }
                if (!yearmonth_valid(d_year, d_month)) {
                    fprintf(stderr,
                        C_RED "오류: 유효하지 않은 연월 %04d-%02d\n" C_RESET,
                        d_year, d_month);
                    return 1;
                }
            }

        /* ── -u ───────────────────────────────────────────────────── */
        } else if (!strcmp(argv[i], "-u")) {
            if (mode == MODE_EDIT) {
                fprintf(stderr,
                    C_RED "오류: -m 은 단독 사용 옵션입니다. "
                    "다른 옵션과 함께 사용할 수 없습니다.\n" C_RESET);
                return 1;
            }
            if (mode == MODE_FMT1) {
                fprintf(stderr,
                    C_RED "오류: -d 와 -u 는 동시에 사용할 수 없습니다.\n"
                    C_RESET);
                return 1;
            }
            mode = MODE_FMT2;

            if (i + 1 < argc && argv[i+1][0] != '-') {
                i++;
                if (sscanf(argv[i], "%d-%d-%d",
                           &u_year, &u_month, &u_day) != 3) {
                    fprintf(stderr,
                        C_RED "오류: -u 날짜 형식은 YYYY-MM-DD 입니다.\n"
                        C_RESET);
                    return 1;
                }
                if (!date_valid(u_year, u_month, u_day)) {
                    fprintf(stderr,
                        C_RED "오류: 유효하지 않은 날짜 %04d-%02d-%02d\n"
                        C_RESET, u_year, u_month, u_day);
                    return 1;
                }
            }

        /* ── -m ───────────────────────────────────────────────────── */
        } else if (!strcmp(argv[i], "-m")) {
            if (mode != MODE_NONE) {
                fprintf(stderr,
                    C_RED "오류: -m 은 단독 사용 옵션입니다. "
                    "다른 옵션과 함께 사용할 수 없습니다.\n" C_RESET);
                return 1;
            }
            mode = MODE_EDIT;

            if (i + 1 < argc && argv[i+1][0] != '-') {
                i++;
                if (sscanf(argv[i], "%d-%d-%d",
                           &u_year, &u_month, &u_day) != 3) {
                    fprintf(stderr,
                        C_RED "오류: -m 날짜 형식은 YYYY-MM-DD 입니다.\n"
                        C_RESET);
                    return 1;
                }
                if (!date_valid(u_year, u_month, u_day)) {
                    fprintf(stderr,
                        C_RED "오류: 유효하지 않은 날짜 %04d-%02d-%02d\n"
                        C_RESET, u_year, u_month, u_day);
                    return 1;
                }
            }

        /* ── -h / --help ──────────────────────────────────────────── */
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;

        /* ── 알 수 없는 옵션 ─────────────────────────────────────── */
        } else {
            fprintf(stderr,
                C_RED "오류: 알 수 없는 옵션 '%s'\n" C_RESET, argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* ── 디스패치 ─────────────────────────────────────────────────────── */
    switch (mode) {
    case MODE_FMT1:
        fmt1_render(d_year, d_month);
        break;

    case MODE_FMT2:
        fmt2_render(u_year, u_month, u_day);
        break;

    case MODE_EDIT:
        /* 진입 전 현재 일정 2형식으로 미리 보기 */
        fmt2_render(u_year, u_month, u_day);
        edit_run(u_year, u_month, u_day);
        /* 편집 후 결과 확인 */
        printf(C_BOLD C_CYAN
               "\n══ 편집 완료 — 최종 일정 ══════════════════════════\n"
               C_RESET);
        fmt2_render(u_year, u_month, u_day);
        break;

    case MODE_NONE:
    default:
        usage(argv[0]);
        break;
    }

    return 0;
}
