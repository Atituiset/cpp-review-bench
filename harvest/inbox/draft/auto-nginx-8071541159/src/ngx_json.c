// AUTO-DRAFT from nginx/nginx PR #1658
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

    if (i) {
        for ( ;; ) {
            len = sizeof("\"\":") - 1 + i->name.len
                  + ngx_escape_json(NULL, i->name.data, i->name.len);

            if (total > NGX_MAX_SIZE_T_VALUE - len) {
                return NGX_ERROR;
            }

/* …（同文件无关代码省略）… */
                return NGX_ERROR;
            }

            if (total > NGX_MAX_SIZE_T_VALUE - len) {
                return NGX_ERROR;
            }

/* …（同文件无关代码省略）… */
                return NGX_ERROR;
            }

            if (total > NGX_MAX_SIZE_T_VALUE - len) {
                return NGX_ERROR;
            }

/* …（同文件无关代码省略）… */
static ssize_t
ngx_json_string_length(ngx_data_item_t *item)
{
    ngx_str_t  *str;

    str = &item->data.string;

    return sizeof("\"\"") - 1 + str->len
           + ngx_escape_json(NULL, str->data, str->len);
}

