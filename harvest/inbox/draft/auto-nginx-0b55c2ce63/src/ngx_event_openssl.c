// AUTO-DRAFT from nginx/nginx PR #1471
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>


        s->data = ngx_pnalloc(pool, s->len);
        if (s->data == NULL) {
            return NGX_ERROR;
        }
