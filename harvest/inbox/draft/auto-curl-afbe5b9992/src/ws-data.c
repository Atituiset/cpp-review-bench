// AUTO-DRAFT from curl/curl PR #11799
CURL *curl;
  CURLcode res = CURLE_OK;
  const char *url;
  curl_off_t l1, l2;  // <<< BUG ANCHOR
  size_t plen_min, plen_max;


  url = argv[1];
  l1 = strtol(argv[2], NULL, 10);
  if(l1 < 0) {
    fprintf(stderr, "minlen must be >= 0, got %ld\n", (long)l1);
    return 2;
  }
  l2 = strtol(argv[3], NULL, 10);
  if(l2 < 0) {
    fprintf(stderr, "maxlen must be >= 0, got %ld\n", (long)l2);
    return 2;
  }
  plen_min = l1;
