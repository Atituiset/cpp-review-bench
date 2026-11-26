// AUTO-DRAFT from nginx/nginx PR #1837
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

            return NULL;
        }

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, path)
            != NGX_OK)
        {
