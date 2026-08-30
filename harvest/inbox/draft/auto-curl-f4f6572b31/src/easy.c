// AUTO-DRAFT from curl/curl PR #15289
* curl_easy_init() is the external interface to alloc, setup and init an
 * easy handle that is returned. If anything goes wrong, NULL is returned.
 */
struct Curl_easy *curl_easy_init(void)
{
  CURLcode result;
  struct Curl_easy *data;
 * Callback that gets called with a new value when the timeout should be
 * updated.
 */
static int events_timer(struct Curl_multi *multi,    /* multi handle */
                        long timeout_ms, /* see above */
                        void *userp)    /* private callback pointer */
{
  struct events *ev = userp;
  (void)multi;
 * Callback that gets called with information about socket activity to
 * monitor.
 */
static int events_socket(struct Curl_easy *easy,      /* easy handle */
                         curl_socket_t s, /* socket */
                         int what,        /* see above */
                         void *userp,     /* private callback
  struct socketmonitor *m;
  struct socketmonitor *prev = NULL;
  bool found = FALSE;

#if defined(CURL_DISABLE_VERBOSE_STRINGS)
  (void) easy;
        else
          ev->list = nxt;
        free(m);
        infof(easy, "socket cb: socket %" FMT_SOCKET_T " REMOVED", s);
      }
      else {
        /* The socket 's' is already being monitored, update the activity
           mask. Convert from libcurl bitmask to the poll one. */
        m->socket.events = socketcb2poll(what);
        infof(easy, "socket cb: socket %" FMT_SOCKET_T
              " UPDATED as %s%s", s,
              (what&CURL_POLL_IN) ? "IN" : "",
              (what&CURL_POLL_OUT) ? "OUT" : "");
  if(!found) {
    if(what == CURL_POLL_REMOVE) {
      /* should not happen if our logic is correct, but is no drama. */
      DEBUGF(infof(easy, "socket cb: asked to REMOVE socket %"
                   FMT_SOCKET_T "but not present!", s));
      DEBUGASSERT(0);
    }
        m->socket.events = socketcb2poll(what);
        m->socket.revents = 0;
        ev->list = m;
        infof(easy, "socket cb: socket %" FMT_SOCKET_
