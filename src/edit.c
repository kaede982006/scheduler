#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "task.h"
#include "fmt2.h"
#include "edit.h"

#define CMD_BUF 1024

/* ── 도움말 ───────────────────────────────────────────────────────────── */
static void print_help(void)
{
    printf(C_BOLD C_CYAN
           "\n──── 편집 명령어 ──────────────────────────────────────\n"
           C_RESET);
    printf("  " C_GREEN "add"  C_RESET "  HH:MM <제목>        일정 추가\n");
    printf("  " C_GREEN "del"  C_RESET "  <ID>                일정 삭제\n");
    printf("  " C_GREEN "edit" C_RESET " <ID> HH:MM <제목>    일정 수정\n");
    printf("  " C_GREEN "list" C_RESET "                       현재 날 일정 목록\n");
    printf("  " C_GREEN "help" C_RESET "                       도움말\n");
    printf("  " C_GREEN "quit" C_RESET " / " C_GREEN "exit" C_RESET
           "               저장 후 종료\n");
    printf("───────────────────────────────────────────────────────\n\n");
}

/* ── list: ID + 일정 출력 ─────────────────────────────────────────────── */
static void list_day(int y, int m, int d)
{
    int         idx[64];
    int         cnt = task_query_day(y, m, d, idx, 64);
    int         dow = day_of_week(y, m, d);
    const char *dc  = (dow == 0 ? C_RED  :
                      (dow == 6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN "%04d-%02d-%02d %s(%s)" C_RESET
           " 일정 [총 %d건]:\n",
           y, m, d, dc, wday_kr[dow], cnt);

    if (cnt == 0) {
        printf(C_DIM "  (등록된 일정 없음)\n\n" C_RESET);
        return;
    }
    for (int i = 0; i < cnt; i++) {
        Task *t = &tasks[idx[i]];
        printf("  " C_DIM "[ID:%3d]" C_RESET
               "  " C_GREEN "%02d:%02d" C_RESET "  %s\n",
               idx[i], t->hour, t->minute, t->title);
    }
    printf("\n");
}

/* ── 시간 파싱 헬퍼: "HH:MM" → hh, mm (1=성공, 0=실패) ──────────────── */
static int parse_time(const char *s, int *hh, int *mm)
{
    return (sscanf(s, "%d:%d", hh, mm) == 2 &&
            *hh >= 0 && *hh <= 23 &&
            *mm >= 0 && *mm <= 59);
}

/* ── 다음 토큰으로 포인터 이동 ───────────────────────────────────────── */
static const char *skip_token(const char *p)
{
    while (*p && *p != ' ') p++;
    while (*p == ' ')       p++;
    return p;
}

/* ═══════════════════════════════════════════════════════════════════════
 * edit_run: 인터랙티브 편집 루프
 * ═══════════════════════════════════════════════════════════════════════ */
void edit_run(int y, int m, int d)
{
    int dow = day_of_week(y, m, d);
    const char *dc = (dow == 0 ? C_RED  :
                     (dow == 6 ? C_BLUE : C_WHITE));

    printf(C_BOLD C_CYAN
           "\n✏  편집 모드: %04d-%02d-%02d %s(%s)\n" C_RESET,
           y, m, d, dc, wday_kr[dow]);
    print_help();
    list_day(y, m, d);

    char line[CMD_BUF];

    for (;;) {
        printf(C_YELLOW "scheduler> " C_RESET);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) {
            printf(C_DIM "\n(EOF) 저장 후 종료합니다.\n" C_RESET);
            task_save();
            return;
        }

        /* 개행 제거 */
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;

        char cmd[32] = {0};
        sscanf(line, "%31s", cmd);
        const char *rest = skip_token(line);  /* cmd 이후 부분 */

        /* ── quit / exit ─────────────────────────────────────────── */
        if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
            printf(C_DIM "저장 후 종료합니다.\n" C_RESET);
            task_save();
            return;
        }

        /* ── help ────────────────────────────────────────────────── */
        if (!strcmp(cmd, "help")) {
            print_help();
            continue;
        }

        /* ── list ────────────────────────────────────────────────── */
        if (!strcmp(cmd, "list")) {
            list_day(y, m, d);
            continue;
        }

        /* ── add HH:MM <제목> ────────────────────────────────────── */
        if (!strcmp(cmd, "add")) {
            int hh, mm;
            if (!parse_time(rest, &hh, &mm)) {
                printf(C_RED "오류: add HH:MM <제목>\n" C_RESET);
                continue;
            }
            const char *title = skip_token(rest);
            if (*title == '\0') {
                printf(C_RED "오류: 제목을 입력하세요.\n" C_RESET);
                continue;
            }
            if (task_add(y, m, d, hh, mm, title) < 0) {
                printf(C_RED "오류: 최대 일정 수(%d) 초과.\n" C_RESET, MAX_TASKS);
                continue;
            }
            printf(C_GREEN "✔ 추가됨: [%02d:%02d] %s\n\n" C_RESET,
                   hh, mm, title);
            list_day(y, m, d);
            continue;
        }

        /* ── del <ID> ────────────────────────────────────────────── */
        if (!strcmp(cmd, "del")) {
            int id = -1;
            if (sscanf(rest, "%d", &id) != 1 || id < 0 || id >= n_tasks) {
                printf(C_RED "오류: del <ID>  —  list 로 ID 확인\n" C_RESET);
                continue;
            }
            Task *t = &tasks[id];
            if (t->year != y || t->month != m || t->day != d) {
                printf(C_RED "오류: ID %d 는 현재 날짜의 일정이 아닙니다.\n"
                       C_RESET, id);
                continue;
            }
            printf(C_RED "✖ 삭제됨: [%02d:%02d] %s\n\n" C_RESET,
                   t->hour, t->minute, t->title);
            task_del(id);
            list_day(y, m, d);
            continue;
        }

        /* ── edit <ID> HH:MM <제목> ──────────────────────────────── */
        if (!strcmp(cmd, "edit")) {
            int id = -1;
            if (sscanf(rest, "%d", &id) != 1 || id < 0 || id >= n_tasks) {
                printf(C_RED "오류: edit <ID> HH:MM <제목>\n" C_RESET);
                continue;
            }
            Task *t = &tasks[id];
            if (t->year != y || t->month != m || t->day != d) {
                printf(C_RED "오류: ID %d 는 현재 날짜의 일정이 아닙니다.\n"
                       C_RESET, id);
                continue;
            }
            const char *timepart = skip_token(rest);
            int hh, mm;
            if (!parse_time(timepart, &hh, &mm)) {
                printf(C_RED "오류: 시간 형식 HH:MM 을 확인하세요.\n" C_RESET);
                continue;
            }
            const char *title = skip_token(timepart);
            if (*title == '\0') {
                printf(C_RED "오류: 제목을 입력하세요.\n" C_RESET);
                continue;
            }
            task_edit(id, hh, mm, title);
            printf(C_GREEN "✔ 수정됨: [%02d:%02d] %s\n\n" C_RESET,
                   hh, mm, title);
            list_day(y, m, d);
            continue;
        }

        printf(C_RED "알 수 없는 명령어: \"%s\"  (help 로 도움말 확인)\n"
               C_RESET, cmd);
    }
}
