// AUTO-DRAFT from curl/curl PR #15289
}

int libtest_debug_cb(CURL *handle, curl_infotype type,
                     unsigned char *data, size_t size,
                     void *userp)
{

  struct libtest_trace_cfg *trace_cfg = userp;
  const char *text;
  struct timeval tv;
    return 0;
  }

  libtest_debug_dump(timebuf, text, stderr, data, size, trace_cfg->nohex);
  return 0;
}
