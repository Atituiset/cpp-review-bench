// AUTO-DRAFT from curl/curl PR #16358
CURLcode Curl_loadhostpairs(struct Curl_easy *data)
{  // <<< BUG ANCHOR
  struct curl_slist *hostp;
  const char *host_end;

  /* Default is no wildcard found */
  data->state.wildcard_resolve = FALSE;

  for(hostp = data->state.resolve; hostp; hostp = hostp->next) {
    char entry_id[MAX_HOSTCACHE_LEN];
    if(!hostp->data)
      continue;
    if(hostp->data[0] == '-') {
      curl_off_t num = 0;
      size_t entry_len;
      size_t hlen = 0;
      host_end = strchr(&hostp->data[1], ':');

      if(host_end) {
        hlen = host_end - &hostp->data[1];
        host_end++;
        if(Curl_str_number(&host_end, &num, 0xffff))
          host_end = NULL;
      }
      if(!host_end) {
        infof(data, "Bad syntax CURLOPT_RESOLVE removal entry '%s'",
              hostp->data);
        continue;
      }
      /* Create an entry id, based upon the hostname and port */
      entry_len = create_hostcache_id(&hostp->data[1], hlen, (int)num,
                                      entry_id, sizeof(entry_id));
      if(data->share)
        Curl_share_lock(data, CURL_LOCK_DATA_DNS, CURL_LOCK_ACCESS_SINGLE);

      /* delete entry, ignore if it did not exist */
      Curl_hash_delete(data->dns.hostcache, entry_id, entry_len + 1);

      if(data->share)
        Curl_share_unlock(data, CURL_LOCK_DATA_DNS);
    }
    else {
      struct Curl_dns_entry *dns;
#if !defined(CURL_DISABLE_VERBOSE_STRINGS)
      const char *addresses = NULL;
#endif
      const char *addr_begin;
      const char *addr_end;
      const char *port_ptr;
      curl_off_t port = 0;
      const char *end_ptr;
      bool permanent = TRUE;
      bool error = TRUE;
      char *host_begin = hostp->data;
      size_t hlen = 0;

      if(host_begin[0] == '+') {
        host_begin++;
        permanent = FALSE;
      }
      host_end = strchr(host_begin, ':');
      if(!host_end)
        goto err;
      hlen = host_end - host_begin;

      port_ptr = host_end + 1;
      if(Curl_str_number(&port_ptr, &port, 0xffff) ||
         (*port_ptr
