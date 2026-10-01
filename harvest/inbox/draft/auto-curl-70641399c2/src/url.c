// AUTO-DRAFT from curl/curl PR #11412
* User name and password set with their own options override the
   * credentials possibly set in the URL.
   */
  if(!data->state.aptr.passwd) {  // <<< BUG ANCHOR
    uc = curl_url_get(uh, CURLUPART_PASSWORD, &data->state.up.password, 0);
    if(!uc) {
      char *decoded;
