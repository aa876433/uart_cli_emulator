//
// Created by JohnLin on 2024/4/20.
//

#include <stdlib.h>
#include <assert.h>
#include "gapbuf.h"

GAP_BUF *gap_buf_create(uint32_t length) {
    GAP_BUF *gap = malloc(sizeof(GAP_BUF) + length);
    gap->all = malloc(length + 1);
    gap->forward = malloc(length + 1);
    gap->total = gap->gap = length;
    gap->front = 0;
    gap->update = 1;
    return gap;
}

void gap_buf_destroy(GAP_BUF *gap) {
    free(gap);
}

void gap_buf_insert(GAP_BUF *gap, char c) {
    if (!gap->gap) {
        return;
    }

    gap->buf[gap->front] = c;
    gap->front++;
    gap->gap--;
}

void gap_buf_backspace(GAP_BUF *gap) {
    assert(gap->front > 0);
    gap->front--;
    gap->gap++;
}

void gap_buf_delete(GAP_BUF *gap) {
    assert(gap->total > (gap->front + gap->gap));
    gap->gap++;
    gap->update = 1;
}

void gap_buf_backward(GAP_BUF *gap) {
    assert(gap->front > 0);
    gap->buf[gap->front + gap->gap - 1] = gap->buf[gap->front - 1];
    gap->front--;
    gap->update = 1;
}

void gap_buf_forward(GAP_BUF *gap) {
    assert(gap->total > (gap->front + gap->gap));
    gap->buf[gap->front] = gap->buf[gap->front + gap->gap];
    gap->front++;
    gap->update = 1;
}

uint8_t gap_buf_can_move(GAP_BUF *gap, uint8_t back) {
    if (back) {
        return gap->front;
    }
    else {
        return gap->total > (gap->front + gap->gap);
    }
}

void gap_buf_move(GAP_BUF *gap, uint8_t back) {
    if (back) {
        gap_buf_backward(gap);
    } else {
        gap_buf_forward(gap);
    }
}

const char *gap_buf_get_all(GAP_BUF *gap) {
    int index = 0;
    for (int i = 0; i < gap->front; i++) {
        gap->all[index++] = gap->buf[i];
    }
    for (int i = gap->front + gap->gap; i < gap->total; i++) {
        gap->all[index++] = gap->buf[i];
    }
    gap->all[index] = 0;
    return gap->all;
}

const char *gap_buf_get_forward(GAP_BUF *gap, int *len) {
    if (gap->update) {
        int index = 0;
        for (int i = gap->front + gap->gap; i < gap->total; i++) {
            gap->forward[index++] = gap->buf[i];
        }
        gap->forward[index] = 0;
    }
    *len = gap->total - (gap->front + gap->gap);
    return gap->forward;
}

void gap_buf_get_len(GAP_BUF *gap, int *front, int *valid) {
    if (front != NULL) {
        *front = gap->front;
    }

    if (valid != NULL) {
        *valid = gap->total - gap->gap;
    }
}

void gap_buf_restore(GAP_BUF *gap, const char *str) {
    gap_buf_reset(gap);
    while (*str != 0) {
        gap_buf_insert(gap, *str);
        str++;
    }
}

void gap_buf_reset(GAP_BUF *gap) {
    gap->front = 0;
    gap->gap = gap->total;
    gap->update = 1;
}
