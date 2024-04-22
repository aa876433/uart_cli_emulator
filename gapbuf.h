//
// Created by JohnLin on 2024/4/20.
//

#ifndef GAPBUF_H
#define GAPBUF_H

#include <stdint.h>

typedef struct {
    uint16_t total;
    uint16_t gap;
    uint16_t front;
    uint16_t update;
    uint16_t space;
    uint16_t rsvd;
    char *forward;
    char *all;
    char buf[0];
} GAP_BUF;

GAP_BUF *gap_buf_create(uint32_t length);
void gap_buf_destroy(GAP_BUF *gap);
void gap_buf_insert(GAP_BUF *gap, char c);
void gap_buf_backspace(GAP_BUF *gap);
void gap_buf_delete(GAP_BUF *gap);
void gap_buf_backward(GAP_BUF *gap);
void gap_buf_forward(GAP_BUF *gap);
uint8_t gap_buf_can_move(GAP_BUF *gap, uint8_t back);
void gap_buf_move(GAP_BUF *gap, uint8_t back);
const char *gap_buf_get_all(GAP_BUF *gap);
const char *gap_buf_get_forward(GAP_BUF *gap, int *len);
void gap_buf_get_len(GAP_BUF *gap, int *front, int *valid);
void gap_buf_restore(GAP_BUF *gap, const char *str);
void gap_buf_reset(GAP_BUF *gap);


#endif //GAPBUF_H
