// AUTO-DRAFT from curl/curl PR #16601
result = setstropt_userpwd(ptr, &u, &p);
  // <<< BUG ANCHOR
    /* URL decode the components */
    if(!result && u)
      result = Curl_urldecode(u, 0, &data->set.str[STRING_PROXYUSERNAME], NULL,
                              REJECT_ZERO);
    if(!result && p)
      result = Curl_urldecode(p, 0, &data->set.str[STRING_PROXYPASSWORD], NULL,
                              REJECT_ZERO);
    free(u);
    free(p);
  }
