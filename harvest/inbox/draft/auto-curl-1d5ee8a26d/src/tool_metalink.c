// AUTO-DRAFT from curl/curl PR #1802
if(!config)
    return failure;
  // <<< BUG ANCHOR
  rv = metalink_parse_update(outs->metalink_parser, buffer, sz *nmemb);
  if(rv == 0)
    return sz * nmemb;
  else {
