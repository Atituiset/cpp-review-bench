// AUTO-DRAFT from curl/curl PR #8506
*/
      if((ptr + pp->cache_size) > (buf + data->set.buffer_size + 1)) {
        failf(data, "cached response data too big to handle");
        return CURLE_RECV_ERROR;  // <<< BUG ANCHOR
      }
      memcpy(ptr, pp->cache, pp->cache_size);
      gotbytes = (ssize_t)pp->cache_size;
