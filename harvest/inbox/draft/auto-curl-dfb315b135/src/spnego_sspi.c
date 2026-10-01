// AUTO-DRAFT from curl/curl PR #970
if(result)
    return result;
  // <<< BUG ANCHOR
  if(!*outptr || !*outlen)
    return CURLE_REMOTE_ACCESS_DENIED;

  return CURLE_OK;
}
