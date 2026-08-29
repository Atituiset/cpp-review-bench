#include "u2u_fields.h"

/* 结构校验：固定头 12 字节，且两段 IMSI 区域完整落在缓冲区内。 */
int u2u_frame_valid(const uint8_t *frame, size_t size) {
    if (!frame || size < 12)
        return 0;
    size_t src_len = frame[10];
    if (size < 12 + src_len)
        return 0;
    size_t dst_len = frame[11 + src_len];
    if (size < 12 + src_len + dst_len)
        return 0;
    return 1;
}

/* 上行业务入口：先校验，后提取。 */
size_t u2u_payload_offset_checked(const uint8_t *frame, size_t size) {
    if (!u2u_frame_valid(frame, size))
        return 0;
    return u2u_payload_offset(frame);
}

/* 告警统计旁路：读取 payload 首字节作类型统计。
 * 入口只查了固定头长度，未做 IMSI 区域结构校验。 */
int u2u_payload_type_peek(const uint8_t *frame, size_t size) {
    if (!frame || size < 12)
        return -1;
    return frame[u2u_payload_offset(frame)];
}
