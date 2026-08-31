// AUTO-DRAFT from curl/curl PR #12041
easy_setopt(ch, CURLOPT_URL, URL);
  easy_setopt(ch, CURLOPT_COOKIEFILE, libtest_arg2);
  res = curl_easy_perform(ch);

test_cleanup:
