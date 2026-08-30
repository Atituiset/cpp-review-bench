// AUTO-DRAFT from curl/curl PR #15289
}

static int progress_callback(void *clientp,
                             double dltotal,
                             double dlnow,
                             double ultotal,
                             double ulnow)
{
  (void)dltotal;
  (void)dlnow;
