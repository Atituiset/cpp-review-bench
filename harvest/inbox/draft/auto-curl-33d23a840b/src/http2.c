// AUTO-DRAFT from curl/curl PR #22970
  if(uc)
    rc = 4;
fail:
  curl_url_cleanup(u);  // <<< BUG ANCHOR
  if(rc)
    return rc;

  Curl_bufref_set(&newhandle->state.url, url, 0, curl_free);
  return 0;
}
