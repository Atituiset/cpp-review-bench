// AUTO-DRAFT from curl/curl PR #15155
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define CPOOL_LOCK(c)                                                   \
  do {                                                                  \
    if((c)) {                                                           \
      if(CURL_SHARE_KEEP_CONNECT((c)->share))                           \
        Curl_share_lock(((c)->idata), CURL_LOCK_DATA_CONNECT,           \
                        CURL_LOCK_ACCESS_SINGLE);                       \
      DEBUGASSERT(!(c)->locked);                                        \
      (c)->locked = TRUE;                                               \
    }                                                                   \
  } while(0)
/* …（同文件无关代码省略）… */
#define CPOOL_UNLOCK(c)                                                 \
  do {                                                                  \
    if((c)) {                                                           \
      DEBUGASSERT((c)->locked);                                         \
      (c)->locked = FALSE;                                              \
      if(CURL_SHARE_KEEP_CONNECT((c)->share))                           \
        Curl_share_unlock((c)->idata, CURL_LOCK_DATA_CONNECT);          \
    }                                                                   \
  } while(0)
/* …（同文件无关代码省略）… */
static void cpool_run_conn_shutdown_handler(struct Curl_easy *data,
                                            struct connectdata *conn)
{
  if(!conn->bits.shutdown_handler) {
    if(conn->dns_entry)
      Curl_resolv_unlink(data, &conn->dns_entry);

    /* Cleanup NTLM connection-related data */
    Curl_http_auth_cleanup_ntlm(conn);

    /* Cleanup NEGOTIATE connection-related data */
    Curl_http_auth_cleanup_negotiate(conn);

    if(conn->handler && conn->handler->disconnect) {
      /* This is set if protocol-specific cleanups should be made */
      DEBUGF(infof(data, "connection #%" FMT_OFF_T
                   ", shutdown protocol handler (aborted=%d)",
                   conn->connection_id, conn->bits.aborted));

      conn->handler->disconnect(data, conn, conn->bits.aborted);
    }

    /* possible left-overs from the async name resolvers */
    Curl_resolver_cancel(data);

    conn->bits.shutdown_handler = TRUE;
  }
}
/* …（同文件无关代码省略）… */
static void cpool_run_conn_shutdown(struct Curl_easy *data,
                                    struct connectdata *conn,
                                    bool *done)
{
  CURLcode r1, r2;
  bool done1, done2;

  /* We expect to be attached when called */
  DEBUGASSERT(data->conn == conn);

  cpool_run_conn_shutdown_handler(data, conn);

  if(conn->bits.shutdown_filters) {
    *done = TRUE;
    return;
  }

  if(!conn->connect_only && Curl_conn_is_connected(conn, FIRSTSOCKET))
    r1 = Curl_conn_shutdown(data, FIRSTSOCKET, &done1);
  else {
    r1 = CURLE_OK;
    done1 = TRUE;
  }

  if(!conn->connect_only && Curl_conn_is_connected(conn, SECONDARYSOCKET))
    r2 = Curl_conn_shutdown(data, SECONDARYSOCKET, &done2);
  else {
    r2 = CURLE_OK;
    done2 = TRUE;
  }

  /* we are done when any failed or both report success */
  *done = (r1 || r2 || (done1 && done2));
  if(*done)
    conn->bits.shutdown_filters = TRUE;
}
/* …（同文件无关代码省略）… */
  return result;
}

CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
                                struct curl_waitfds *cwfds)
{
  CURLcode result = CURLE_OK;

  CPOOL_LOCK(cpool);
  if(Curl_llist_head(&cpool->shutdowns)) {
/* …（同文件无关代码省略）… */
      Curl_conn_adjust_pollset(cpool->idata, &ps);
      Curl_detach_connection(cpool->idata);

      result = Curl_waitfds_add_ps(cwfds, &ps);
      if(result)
        goto out;
    }
  }
out:
  CPOOL_UNLOCK(cpool);
  return result;
}

static void cpool_perform(struct cpool *cpool)
{
  struct Curl_easy *data = cpool->idata;
  struct Curl_llist_node *e = Curl_llist_head(&cpool->shutdowns);
  struct Curl_llist_node *enext;
  struct connectdata *conn;
  struct curltime *nowp = NULL;
  struct curltime now;
  timediff_t next_from_now_ms = 0, ms;
  bool done;

  if(!e)
    return;

  DEBUGASSERT(data);
  DEBUGF(infof(data, "[CCACHE] perform, %zu connections being shutdown",
               Curl_llist_count(&cpool->shutdowns)));
  while(e) {
    enext = Curl_node_next(e);
    conn = Curl_node_elem(e);
    Curl_attach_connection(data, conn);
    cpool_run_conn_shutdown(data, conn, &done);
    DEBUGF(infof(data, "[CCACHE] shutdown #%" FMT_OFF_T ", done=%d",
                 conn->connection_id, done));
    Curl_detach_connection(data);
    if(done) {
      Curl_node_remove(e);
      cpool_close_and_destroy(cpool, conn, NULL, FALSE);
    }
    else {
      /* Not done, when does this connection time out? */
      if(!nowp) {
        now = Curl_now();
        nowp = &now;
      }
      ms = Curl_conn_shutdown_timeleft(conn, nowp);
      if(ms && ms < next_from_now_ms)
        next_from_now_ms = ms;
    }
    e = enext;
  }

  if(next_from_now_ms)
    Curl_expire(data, next_from_now_ms, EXPIRE_RUN_NOW);
}
/* …（同文件无关代码省略）… */
static void cpool_close_and_destroy(struct cpool *cpool,
                                    struct connectdata *conn,
                                    struct Curl_easy *data,
                                    bool do_shutdown)
{
  bool done;

  /* there must be a connection to close */
  DEBUGASSERT(conn);
  /* it must be removed from the connection pool */
  DEBUGASSERT(!conn->bits.in_cpool);
  /* there must be an associated transfer */
  DEBUGASSERT(data || cpool);
  if(!data)
    data = cpool->idata;

  /* the transfer must be detached from the connection */
  DEBUGASSERT(data && !data->conn);

  Curl_attach_connection(data, conn);

  cpool_run_conn_shutdown_handler(data, conn);
  if(do_shutdown) {
    /* Make a last attempt to shutdown handlers and filters, if
     * not done so already. */
    cpool_run_conn_shutdown(data, conn, &done
