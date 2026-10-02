// AUTO-DRAFT from nginx/nginx PR #1756
#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_BPF_LOGBUF_SIZE  (16 * 1024)


static ngx_inline int
