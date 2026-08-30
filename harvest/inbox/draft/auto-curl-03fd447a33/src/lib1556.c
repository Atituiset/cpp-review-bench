// AUTO-DRAFT from curl/curl PR #15289
size_t largest;
};

static size_t header(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t headersize = size * nmemb;
  struct headerinfo *info = (struct headerinfo *)stream;
