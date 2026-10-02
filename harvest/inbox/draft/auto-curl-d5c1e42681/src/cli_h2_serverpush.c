// AUTO-DRAFT from curl/curl PR #22970
      m = curl_multi_info_read(multi, &msgq);
      if(m && (m->msg == CURLMSG_DONE)) {
        CURL *easy = m->easy_handle;
        transfers--;
        curl_multi_remove_handle(multi, easy);
        curl_easy_cleanup(easy);
