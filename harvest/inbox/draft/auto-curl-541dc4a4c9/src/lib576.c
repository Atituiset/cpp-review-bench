// AUTO-DRAFT from curl/curl PR #15289
};

static
long chunk_bgn(const struct curl_fileinfo *finfo, void *ptr, int remains)
{
  struct chunk_data *ch_d = ptr;
  ch_d->remains = remains;
