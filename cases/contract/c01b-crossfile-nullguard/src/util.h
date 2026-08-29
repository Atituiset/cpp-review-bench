#ifndef C01B_UTIL_H
#define C01B_UTIL_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t  tag;
    uint16_t len;
    uint8_t  payload[128];
} Pkt;

/* 跨文件契约：调用方必须先调 validate()，非空且 len 受控才解引用 */
int validate(Pkt *p);
void consume(Pkt *p);

#endif /* C01B_UTIL_H */
