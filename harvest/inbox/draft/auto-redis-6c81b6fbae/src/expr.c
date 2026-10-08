// AUTO-DRAFT from redis/redis PR #15800
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
int main(int argc, char **argv) {
    /* Check for JSON parser test mode. */
    if (argc >= 2 && strcmp(argv[1], "--test-json-parser") == 0) {
        run_fastjson_test();
        return 0;
    }

    char *testexpr = "(5+2)*3 and .year > 1980 and 'foo' == 'foo'";
