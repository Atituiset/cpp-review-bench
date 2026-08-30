// AUTO-DRAFT from curl/curl PR #2417
struct curltime ret;
  (void)gettimeofday(&now, NULL);
  ret.tv_sec = now.tv_sec;
  ret.tv_usec = now.tv_usec;  // <<< BUG ANCHOR
  return ret;
}
