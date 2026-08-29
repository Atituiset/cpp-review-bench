#include <stddef.h>
#include <stdint.h>

/* IMSI 数字串 → packed BCD 编码（TS 24.008 §10.5.1.4）。
 * digits 指向报文中解析出的数字字段，恰好 n 字节、非 NUL 结尾。
 * 奇数个数字时，最后字节的高半字节应填 0xF。 */
size_t imsi_bcd_encode(const uint8_t *digits, size_t n, uint8_t *out, size_t cap) {
    size_t j = 0;
    for (size_t i = 0; i < n; i += 2) {
        uint8_t hi = (uint8_t)(digits[i] - '0');
        uint8_t lo = (uint8_t)(digits[i + 1] - '0');
        if (j < cap)
            out[j] = (uint8_t)((lo << 4) | hi);
        ++j;
    }
    return j;
}
