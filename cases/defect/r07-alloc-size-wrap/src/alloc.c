/* r07-alloc-size-wrap：malloc(n*size) 乘法回绕 → 小分配大写入 cwe-190+787 */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

/* n 个元素，每元素 size 字节，乘法回绕导致 malloc 过小，随后越界写 */
void *r07_alloc(uint32_t n, uint32_t size)
{
    /* 锚点（must_find）：n*size 为 uint32_t，n 大时乘法回绕为小值，
       malloc 分配不足，后续按 n*size 写入越界（cwe-190+787） */
    size_t total = (size_t)(n * size);   /* 先 uint32 乘再转 size_t，回绕已发生 */
    uint8_t *p = (uint8_t *)malloc(total);
    if (p == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; i < n * size; i++) {   /* 越界写 */
        p[i] = 0;
    }
    return p;
}

/* 安全点（must_not_find）：正确用 size_t 乘法并受界 */
void *r07_alloc_ok(uint32_t n, uint32_t size)
{
    size_t total = (size_t)n * (size_t)size;   /* 锚点（must_not_find）：size_t 乘法不回绕 */
    if (total > 1024 * 1024) {
        return NULL;
    }
    uint8_t *p = (uint8_t *)malloc(total);
    if (p) {
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return p;
}
