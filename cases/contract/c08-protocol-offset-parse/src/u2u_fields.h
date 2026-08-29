#ifndef U2U_FIELDS_H
#define U2U_FIELDS_H

#include <stddef.h>
#include <stdint.h>

/* U2U 上行帧 wire 格式：
 *   [0..9]   固定头（协议号/序列号/时间戳等）
 *   [10]     src_imsi_len
 *   [11..]   src_imsi（src_imsi_len 字节）
 *   [..]     dst_imsi_len（1 字节）
 *   [..]     dst_imsi（dst_imsi_len 字节）
 *   其后为 payload。
 *
 * 本模块的字段访问函数不做边界检查：调用方必须先通过
 * u2u_frame_valid() 完成结构校验（入口统一校验契约）。 */

/* 结构校验：固定头与两段 IMSI 区域完整落在 size 内返回非 0 */
int u2u_frame_valid(const uint8_t *frame, size_t size);

/* payload 起始偏移（仅可在 u2u_frame_valid 通过后调用） */
size_t u2u_payload_offset(const uint8_t *frame);

#endif
