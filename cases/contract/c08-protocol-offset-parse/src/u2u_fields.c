#include "u2u_fields.h"

/* 字段提取实现。以下读取均依赖调用方已完成 u2u_frame_valid() 校验，
 * 本文件不做任何边界检查。 */

static uint8_t src_len_at(const uint8_t *frame) {
    return frame[10];
}

static uint8_t dst_len_at(const uint8_t *frame) {
    return frame[11 + src_len_at(frame)];
}

static size_t imsi_region_end(const uint8_t *frame) {
    return 12 + src_len_at(frame) + dst_len_at(frame);
}

size_t u2u_payload_offset(const uint8_t *frame) {
    return imsi_region_end(frame);
}
