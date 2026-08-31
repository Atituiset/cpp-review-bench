// AUTO-DRAFT from curl/curl PR #15155
CURLMcode result = CURLM_OK;
  struct Curl_llist_node *e;
  struct Curl_multi *multi = m;

  if(!ufds)
    return CURLM_BAD_FUNCTION_ARGUMENT;

  if(!GOOD_MULTI_HANDLE(multi))
  for(e = Curl_llist_head(&multi->process); e; e = Curl_node_next(e)) {
    struct Curl_easy *data = Curl_node_elem(e);
    multi_getsock(data, &data->last_poll);
    if(Curl_waitfds_add_ps(&cwfds, &data->last_poll)) {
      result = CURLM_OUT_OF_MEMORY;
      goto out;
    }
  }

  if(Curl_cpool_add_waitfds(&multi->cpool, &cwfds)) {
    result = CURLM_OUT_OF_MEMORY;
    goto out;
  }

out:
  if(fd_count)
    *fd_count = cwfds.n;
  return result;
}
