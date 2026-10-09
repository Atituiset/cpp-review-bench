// AUTO-DRAFT from curl/curl PR #16601
  test_setopt(curl, CURLOPT_PROXYAUTH,
              (long) (CURLAUTH_BASIC | CURLAUTH_DIGEST | CURLAUTH_NTLM));
  test_setopt(curl, CURLOPT_PROXY, libtest_arg2); /* set in first.c */
  test_setopt(curl, CURLOPT_PROXYUSERPWD, "me:password");

  res = curl_easy_perform(curl);
