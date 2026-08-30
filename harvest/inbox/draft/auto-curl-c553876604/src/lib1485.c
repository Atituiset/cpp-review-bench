// AUTO-DRAFT from curl/curl PR #15289
int http_status;
};

static size_t header_callback(void *ptr, size_t size, size_t nmemb,
                              void *userp)
{
  struct transfer_status *st = (struct transfer_status *)userp;
  return len;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct transfer_status *st = (struct transfer_status *)userp;
  size_t len = size * nmemb;
