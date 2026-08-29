#include <stddef.h>
#include <stdint.h>

/* NAS GUTI 信息元（IE）编码尺寸计算。
 * wire 格式：每个 IE 为 tag(1B) + len(1B) + value(len B)。 */

typedef struct {
    uint8_t tag;
    uint8_t len;
} IeField;

/* 字段读取。依赖上游契约：ie 非空（见 guti_encode_size 入口判空）。 */
static int ie_field_len(const IeField *ie) {
    return ie->len;
}

static int ie_wire_size(const IeField *ie) {
    return ie_field_len(ie) + 2;   /* tag + len 两字节头 */
}

static int guti_payload_size(const IeField *guti) {
    return ie_wire_size(guti) + 3; /* mcc/mnc 三字节 */
}

static int guti_body_size(const IeField *guti) {
    return guti_payload_size(guti) + 1; /* iea 字节 */
}

/* 业务入口：空指针由本层统一防御，下游链路不再重复判空。 */
int guti_encode_size(const IeField *guti) {
    if (!guti)
        return -1;
    return guti_body_size(guti);
}

/* 整组 IE 的编码总长，调用方按返回值分配拼接缓冲区。 */
int guti_group_size(const IeField *ies, int count) {
    uint8_t total = 0;
    for (int i = 0; i < count; ++i)
        total += (uint8_t)ie_wire_size(&ies[i]);
    return total;
}
