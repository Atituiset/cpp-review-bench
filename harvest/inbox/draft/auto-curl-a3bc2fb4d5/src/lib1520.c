// AUTO-DRAFT from curl/curl PR #23604
  easy_setopt(curl, CURLOPT_MAIL_FROM, "<sender@example.com>");
  easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpt_list);
  easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  result = curl_easy_perform(curl);
