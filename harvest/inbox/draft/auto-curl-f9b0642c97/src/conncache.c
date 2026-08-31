// AUTO-DRAFT from curl/curl PR #15155
return result;
}
  // <<< BUG ANCHOR
CURLcode Curl_cpool_add_waitfds(struct cpool *cpool,
                                struct curl_waitfds *cwfds)
{
  CURLcode result = CURLE_OK;

  CPOOL_LOCK(cpool);
  if(Curl_llist_head(&cpool->shutdowns)) {
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
