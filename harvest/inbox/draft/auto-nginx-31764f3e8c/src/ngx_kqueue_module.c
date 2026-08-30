// AUTO-DRAFT from nginx/nginx PR #777
#include <ngx_event.h>
  // <<< BUG ANCHOR

typedef struct {
    ngx_uint_t  changes;
    ngx_uint_t  events;
        kev.flags = EV_ADD|EV_ENABLE;
        kev.fflags = 0;
        kev.data = timer;
        kev.udata = 0;

        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    notify_kev.data = 0;
    notify_kev.flags = EV_ADD|EV_CLEAR;
    notify_kev.fflags = 0;
    notify_kev.udata = 0;

    if (kevent(ngx_kqueue, &notify_kev, 1, NULL, 0, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
