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
#define OPT(o, d) \
    printf("  " C_YELLOW "%s" C_RESET "\n      %s\n", (o), (d))

static void usage(const char *prog)
{
    printf(C_BOLD "\n  %s" C_RESET " — CLI 스케줄러\n\n", prog);

    OPT("(인자 없음) / -M [YYYY-MM]",
        "1형식: 월간 달력 뷰");
    OPT("-D [YYYY-MM-DD]",
        "2형식: 일간 뷰");
    printf("\n");
    OPT("-t YYYY-MM-DD <경로> [<경로> ...]",
        "일정 추가  (경로 예: 팀/회의  또는  \"팀 회의\"/준비)");
    OPT("-r YYYY-MM-DD <경로> [<경로> ...]",
        "일정 삭제  (하위 경로 cascade 삭제)");
    OPT("-c YYYY-MM-DD <경로> <N>",
        "N일마다 반복 설정 (0=해제)  ※ 루트 노드만 허용");
    printf("\n");
    OPT("-p YYYY-MM-DD <경로>",
        "설명 출력");
    OPT("-i YYYY-MM-DD <경로> <설명>",
        "설명 추가/수정");
    OPT("-d YYYY-MM-DD <경로>",
        "설명 삭제");
    printf("\n");
    OPT("-h / --help", "도움말");
    printf("\n");
    printf("  경로 구분자: /   예) 팀/회의/아젠다\n");
    printf("  날짜 생략 시 오늘 날짜를 사용합니다.\n");
    printf("  각 옵션은 독립적으로 수행됩니다.\n\n");
}

#undef OPT

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

static void err_args(const char *opt, const char *fmt)
{
    fprintf(stderr, C_RED "오류: %s %s\n" C_RESET, opt, fmt);
}

static void err_date(const char *opt, const char *raw)
{
    fprintf(stderr,
        C_RED "오류: %s — 유효하지 않은 날짜 '%s'\n" C_RESET, opt, raw);
}

/*
 * normalize_path: 경로 정규화
 *   - 빈 segment 제거 (연속 슬래시 // 처리)
 *   - 선행/후행 슬래시 제거
 *   - 각 segment 의 양쪽 공백 제거
 *
 * 반환: 정상 경로이면 1, 빈 결과이면 0
 */
static int normalize_path(const char *raw, char *out, int cap)
{
    char buf[TITLE_LEN * 2];
    snprintf(buf, sizeof buf, "%s", raw);

    char result[TITLE_LEN];
    result[0] = '\0';
    int rlen   = 0;

    char *tok = strtok(buf, "/");
    while (tok) {
        /* 선행 공백 제거 */
        while (*tok == ' ') tok++;
        /* 후행 공백 제거 */
        int tlen = (int)strlen(tok);
        while (tlen > 0 && tok[tlen - 1] == ' ') tlen--;
        tok[tlen] = '\0';

        if (tlen == 0) { tok = strtok(NULL, "/"); continue; } /* 빈 segment */

        if (rlen > 0) {
            if (rlen + 1 >= cap) break;
            result[rlen++] = '/';
            result[rlen]   = '\0';
        }
        if (rlen + tlen >= cap) break;
        memcpy(result + rlen, tok, (size_t)tlen);
        rlen += tlen;
        result[rlen] = '\0';

        tok = strtok(NULL, "/");
    }

    if (rlen == 0) return 0;
    snprintf(out, cap, "%s", result);
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -M
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_M(int argc, char *argv[], int i)
{
    int cy, cm, cd; today(&cy, &cm, &cd);
    int y = cy, m = cm;
    if (i < argc && argv[i][0] != '-') {
        if (!parse_yearmonth(argv[i], &y, &m)) { err_date("-M", argv[i]); return 1; }
    }
    fmt1_render(y, m); return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -D
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_D(int argc, char *argv[], int i)
{
    int cy, cm, cd; today(&cy, &cm, &cd);
    int y = cy, m = cm, d = cd;
    if (i < argc && argv[i][0] != '-') {
        if (!parse_date(argv[i], &y, &m, &d)) { err_date("-D", argv[i]); return 1; }
    }
    fmt2_render(y, m, d); return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -t  YYYY-MM-DD  <경로> [<경로> ...]
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_t(int argc, char *argv[], int i)
{
    if (i >= argc) { err_args("-t", "YYYY-MM-DD <경로> [...]"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-t", argv[i]); return 1; }
    i++;

    if (i >= argc) { err_args("-t", "경로를 하나 이상 입력하세요."); return 1; }

    int changed = 0, errors = 0;

    while (i < argc && argv[i][0] != '-') {
        char path[TITLE_LEN];
        if (!normalize_path(argv[i++], path, TITLE_LEN)) {
            fprintf(stderr, C_RED "오류: 빈 경로는 허용되지 않습니다.\n" C_RESET);
            errors++; continue;
        }

        int rc = task_upsert(y, m, d, path);
        switch (rc) {
        case 0:
            printf(C_GREEN "✔ 추가됨: %04d-%02d-%02d  %s\n" C_RESET, y, m, d, path);
            changed++; break;
        case -2:
            fprintf(stderr, C_YELLOW "  이미 존재: %04d-%02d-%02d  %s\n" C_RESET, y, m, d, path);
            errors++; break;
        default:
            fprintf(stderr, C_RED "오류: 최대 일정 수(%d) 초과.\n" C_RESET, MAX_TASKS);
            return 1;
        }
    }

    if (changed > 0) { task_save(); printf(C_DIM "  → %d건 저장됨\n" C_RESET, changed); }
    return (errors > 0 && changed == 0) ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -r  YYYY-MM-DD  <경로> [<경로> ...]
 *   부모 경로 삭제 시 하위 경로도 cascade 삭제됨
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_r(int argc, char *argv[], int i)
{
    if (i >= argc) { err_args("-r", "YYYY-MM-DD <경로> [...]"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-r", argv[i]); return 1; }
    i++;

    if (i >= argc) { fprintf(stderr, C_RED "오류: 삭제할 경로를 입력하세요.\n" C_RESET); return 1; }

    int total_removed = 0;

    while (i < argc && argv[i][0] != '-') {
        char path[TITLE_LEN];
        if (!normalize_path(argv[i++], path, TITLE_LEN)) continue;

        int cnt = task_del_by_title(y, m, d, path);
        if (cnt == 0)
            printf(C_YELLOW "  없음: %04d-%02d-%02d  '%s'\n" C_RESET, y, m, d, path);
        else
            printf(C_RED "✖ 삭제됨: %04d-%02d-%02d  %s  (%d건)\n" C_RESET, y, m, d, path, cnt);
        total_removed += cnt;
    }

    if (total_removed > 0) { task_save(); printf(C_DIM "  → 총 %d건 삭제됨\n" C_RESET, total_removed); }
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -c  YYYY-MM-DD  <경로>  <N>
 *   ※ 루트 노드(깊이 0)만 허용. 자식 노드에는 반복 설정 불가.
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_c(int argc, char *argv[], int i)
{
    if (i + 2 >= argc) { err_args("-c", "YYYY-MM-DD <경로> <N>"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-c", argv[i]); return 1; }
    i++;

    char path[TITLE_LEN];
    if (!normalize_path(argv[i++], path, TITLE_LEN)) {
        fprintf(stderr, C_RED "오류: 빈 경로는 허용되지 않습니다.\n" C_RESET); return 1;
    }

    /* ── 루트 노드 여부 검사 ─────────────────────────────────────────── */
    if (path_depth(path) != 0) {
        fprintf(stderr,
            C_RED "오류: -c 는 루트 노드(최상위)에만 설정할 수 있습니다.\n"
            "      '%s' 는 자식 노드입니다.\n" C_RESET, path);
        return 1;
    }

    int n = -1;
    if (sscanf(argv[i], "%d", &n) != 1 || n < 0) {
        fprintf(stderr, C_RED "오류: N 은 0 이상의 정수여야 합니다.\n" C_RESET); return 1;
    }

    int idx = task_find_by_title(y, m, d, path);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' — %04d-%02d-%02d 에 해당 일정 없음\n" C_RESET,
            path, y, m, d);
        return 1;
    }

    task_set_repeat(idx, n);
    task_save();

    if (n == 0)
        printf(C_CYAN "↻ 반복 해제: %04d-%02d-%02d  %s\n" C_RESET, y, m, d, path);
    else
        printf(C_CYAN "↻ 반복 설정: %04d-%02d-%02d  %s  — %d일마다\n" C_RESET, y, m, d, path, n);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -p  YYYY-MM-DD  <경로>
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_p(int argc, char *argv[], int i)
{
    if (i + 1 >= argc) { err_args("-p", "YYYY-MM-DD <경로>"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-p", argv[i]); return 1; }
    i++;

    char path[TITLE_LEN];
    if (!normalize_path(argv[i], path, TITLE_LEN)) {
        fprintf(stderr, C_RED "오류: 빈 경로.\n" C_RESET); return 1;
    }

    int idx = task_find_by_title(y, m, d, path);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' — %04d-%02d-%02d 에 해당 일정 없음\n" C_RESET,
            path, y, m, d);
        return 1;
    }

    if (tasks[idx].desc[0] == '\0') { printf(C_DIM "  (설명 없음)\n" C_RESET); return 0; }
    printf(C_BOLD C_CYAN "◆ 설명: %04d-%02d-%02d  %s\n" C_RESET, y, m, d, path);
    hr_line(52);
    printf("  %s\n\n", tasks[idx].desc);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -i  YYYY-MM-DD  <경로>  <설명>
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_i(int argc, char *argv[], int i)
{
    if (i + 2 >= argc) { err_args("-i", "YYYY-MM-DD <경로> <설명>"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-i", argv[i]); return 1; }
    i++;

    char path[TITLE_LEN];
    if (!normalize_path(argv[i++], path, TITLE_LEN)) {
        fprintf(stderr, C_RED "오류: 빈 경로.\n" C_RESET); return 1;
    }
    const char *desc = argv[i];

    int idx = task_find_by_title(y, m, d, path);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' — %04d-%02d-%02d 에 해당 일정 없음\n" C_RESET,
            path, y, m, d);
        return 1;
    }

    const char *action = (tasks[idx].desc[0] != '\0') ? "수정" : "추가";
    task_set_desc(idx, desc);
    task_save();
    printf(C_GREEN "✔ 설명 %s됨: %04d-%02d-%02d  %s\n" C_RESET, action, y, m, d, path);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * 핸들러 — -d  YYYY-MM-DD  <경로>   (설명 삭제)
 * ═══════════════════════════════════════════════════════════════════════ */
static int handle_d(int argc, char *argv[], int i)
{
    if (i + 1 >= argc) { err_args("-d", "YYYY-MM-DD <경로>"); return 1; }

    int y, m, d;
    if (!parse_date(argv[i], &y, &m, &d)) { err_date("-d", argv[i]); return 1; }
    i++;

    char path[TITLE_LEN];
    if (!normalize_path(argv[i], path, TITLE_LEN)) {
        fprintf(stderr, C_RED "오류: 빈 경로.\n" C_RESET); return 1;
    }

    int idx = task_find_by_title(y, m, d, path);
    if (idx < 0) {
        fprintf(stderr,
            C_RED "오류: '%s' — %04d-%02d-%02d 에 해당 일정 없음\n" C_RESET,
            path, y, m, d);
        return 1;
    }

    if (tasks[idx].desc[0] == '\0') { printf(C_DIM "  (이미 설명이 없습니다)\n" C_RESET); return 0; }
    task_clear_desc(idx);
    task_save();
    printf(C_RED "✖ 설명 삭제됨: %04d-%02d-%02d  %s\n" C_RESET, y, m, d, path);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    task_init_path();
    task_load();

    if (argc < 2) {
        int cy, cm, cd; today(&cy, &cm, &cd);
        fmt1_render(cy, cm); return 0;
    }

    const char *opt  = argv[1];
    int         rest = 2;

    if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) { usage(argv[0]); return 0; }
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
