/* r07-alloc-size-wrap：按元素个数与元素大小分配并清零缓冲 */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

/* n 个元素，每元素 size 字节，分配后逐字节清零 */
void *r07_alloc(uint32_t n, uint32_t size)
{
    /* 计算总字节数并分配 */
    size_t total = (size_t)(n * size);   /* 总字节数 */
    uint8_t *p = (uint8_t *)malloc(total);
    if (p == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; i < n * size; i++) {   /* 逐字节清零 */
        p[i] = 0;
    }
    return p;
}

/* 同上，乘法先提升为 size_t，并限制单次分配上限 */
void *r07_alloc_ok(uint32_t n, uint32_t size)
{
    size_t total = (size_t)n * (size_t)size;   /* 总字节数 */
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
