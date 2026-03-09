#ifndef FMT1_H
#define FMT1_H

/*
 * fmt1_text_lines: 월간 달력 — 텍스트 라인 형식
 * fmt1_tree      : 월간 달력 — 트리 형식
 */
void fmt1_text_lines(int year, int month);
void fmt1_tree      (int year, int month);

/* 두 형식을 순서대로 모두 출력하는 래퍼 */
void fmt1_render    (int year, int month);

#endif /* FMT1_H */
