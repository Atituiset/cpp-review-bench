// AUTO-DRAFT from curl/curl PR #15289
#ifdef HAVE_LIBSSH2_KNOWNHOST_API
static int sshkeycallback(struct Curl_easy *easy,
                          const struct curl_khkey *knownkey, /* known */
                          const struct curl_khkey *foundkey, /* found */
                          enum curl_khmatch match,
