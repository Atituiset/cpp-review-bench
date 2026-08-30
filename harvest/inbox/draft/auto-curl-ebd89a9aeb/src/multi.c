// AUTO-DRAFT from curl/curl PR #15289
return NULL;
}

struct Curl_multi *curl_multi_init(void)
{
  return Curl_multi_handle(CURL_SOCKET_HASH_TABLE_SIZE,
                           CURL_CONNECTION_HASH_SIZE,
#define multi_warn_debug(x,y) Curl_nop_stmt
#endif

CURLMcode curl_multi_add_handle(struct Curl_multi *multi,
                                struct Curl_easy *data)
{
  CURLMcode rc;
  /* First, make some basic checks that the CURLM handle is a good handle */
  if(!GOOD_MULTI_HANDLE(multi))
    return CURLM_BAD_HANDLE;
    connclose(conn, "Removing connect-only easy handle");
}

CURLMcode curl_multi_remove_handle(struct Curl_multi *multi,
                                   struct Curl_easy *data)
{
  struct Curl_easy *easy = data;
  bool premature;
  struct Curl_llist_node *e;
  CURLMcode rc;

  /* This ignores the return code even in case of problems because there is
     nothing more to do about that, here */
  (void)singlesocket(multi, easy); /* to let the application know what sockets
                                      that vanish with this handle */

  /* Remove the association between the connection and the handle */
  for(e = Curl_llist_head(&multi->msglist); e; e = Curl_node_next(e)) {
    struct Curl_message *msg = Curl_node_elem(e);

    if(msg->extmsg.easy_handle == easy) {
      Curl_node_remove(e);
      /* there can only be one from this specific handle */
      break;
  }
}

CURLMcode curl_multi_fdset(struct Curl_multi *multi,
                           fd_set *read_fd_set, fd_set *write_fd_set,
                           fd_set *exc_fd_set, int *max_fd)
{
     and then we must make sure that is done. */
  int this_max_fd = -1;
  struct Curl_llist_node *e;
  (void)exc_fd_set; /* not used */

  if(!GOOD_MULTI_HANDLE(multi))
  return CURLM_OK;
}

CURLMcode curl_multi_waitfds(struct Curl_multi *multi,
                             struct curl_waitfd *ufds,
                             unsigned int size,
                             unsigned int *fd_count)
{
  struct curl_waitfds cwfds;
