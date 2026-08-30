/* KLEE 符号执行入口：符号化 arr/idx，驱动 r11_get 提交版（STAGED_TREE=0）无边界检查越界读。
 * 真实缺陷：提交版此处无 if(idx>=16) 检查，arr[idx] 越界（build/集成类缺陷）。
 */
#include <stdint.h>
#include <klee/klee.h>

uint8_t r11_get(const uint8_t *arr, uint8_t idx);

int main(void)
{
    uint8_t arr[16];
    uint8_t idx;
    klee_make_symbolic(arr, sizeof(arr), "arr");
    klee_make_symbolic(&idx, sizeof(idx), "idx");
    (void)r11_get(arr, idx);
    return 0;
}
