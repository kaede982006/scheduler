#ifndef EDIT_H
#define EDIT_H

/*
 * edit_run: -m 전용 인터랙티브 편집 루프
 *
 *   진입 시: 해당 날짜의 현재 일정을 2형식으로 출력
 *   종료 시: tasks.csv 저장 후 최종 일정 재표시
 *
 *   내부 명령어:
 *     add  HH:MM <제목>
 *     del  <ID>
 *     edit <ID> HH:MM <제목>
 *     list
 *     help
 *     quit / exit
 */
void edit_run(int year, int month, int day);

#endif /* EDIT_H */
