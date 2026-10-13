// AUTO-DRAFT from nginx/nginx PR #348
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>


    wbio = BIO_new(BIO_s_null());
    if (wbio == NULL) {
        return 0;
    }
