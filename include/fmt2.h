#ifndef FMT2_H
#define FMT2_H

/*
 * fmt2_text_lines: 일간 시간 뷰 — 텍스트 라인 형식
 * fmt2_tree      : 일간 시간 뷰 — 트리 형식
 */
void fmt2_text_lines(int year, int month, int day);
void fmt2_tree      (int year, int month, int day);

/* 두 형식을 순서대로 모두 출력하는 래퍼 */
void fmt2_render    (int year, int month, int day);

#endif /* FMT2_H */
