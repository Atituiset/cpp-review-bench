// AUTO-DRAFT from curl/curl PR #15155
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
                       unsigned int static_count)
{
  DEBUGASSERT(cwfds);
  DEBUGASSERT(static_wfds);
  memset(cwfds, 0, sizeof(*cwfds));
  cwfds->wfds = static_wfds;
  cwfds->count = static_count;
}

static CURLcode cwfds_add_sock(struct curl_waitfds *cwfds,
                               curl_socket_t sock, short events)
{
  int i;

  if(cwfds->n <= INT_MAX) {
    for(i = (int)cwfds->n - 1; i >= 0; --i) {
      if(sock == cwfds->wfds[i].fd) {
        cwfds->wfds[i].events |= events;
        return CURLE_OK;
      }
    }
  }
  /* not folded, add new entry */
  if(cwfds->n >= cwfds->count)
    return CURLE_OUT_OF_MEMORY;
  cwfds->wfds[cwfds->n].fd = sock;
  cwfds->wfds[cwfds->n].events = events;
  ++cwfds->n;
  return CURLE_OK;
}

CURLcode Curl_waitfds_add_ps(struct curl_waitfds *cwfds,
                             struct easy_pollset *ps)
{
  size_t i;

  DEBUGASSERT(cwfds);
  DEBUGASSERT(ps);
/* …（同文件无关代码省略）… */
      events |= CURL_WAIT_POLLIN;
    if(ps->actions[i] & CURL_POLL_OUT)
      events |= CURL_WAIT_POLLOUT;
    if(events) {
      if(cwfds_add_sock(cwfds, ps->sockets[i], events))
        return CURLE_OUT_OF_MEMORY;
    }
  }
  return CURLE_OK;
}
