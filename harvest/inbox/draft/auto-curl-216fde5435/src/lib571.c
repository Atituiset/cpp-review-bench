// AUTO-DRAFT from curl/curl PR #15289
static int rtp_packet_count = 0;

static size_t rtp_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
  char *data = (char *)ptr;
  int channel = RTP_PKT_CHANNEL(data);
