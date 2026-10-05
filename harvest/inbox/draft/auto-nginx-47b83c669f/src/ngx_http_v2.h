// AUTO-DRAFT from nginx/nginx PR #1441
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>

    ngx_uint_t                       sid;
    size_t                           length;
    size_t                           padding;
    unsigned                         flags:8;

    unsigned                         incomplete:1;
