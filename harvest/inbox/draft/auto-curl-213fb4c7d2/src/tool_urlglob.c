// AUTO-DRAFT from curl/curl PR #1098e1044e6aadb471999c88bd184132c3ebc3d8
done = TRUE;
      FALLTHROUGH();
    case ',':
      if(size >= 100000)  // <<< BUG ANCHOR
        return globerror(glob, "range overflow", 0, CURLE_URL_MALFORMAT);

      if(!palloc) {
        palloc = 5; /* a reasonable default */
