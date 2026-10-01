// AUTO-DRAFT from curl/curl PR #115
return CURLE_OK;
}
  // <<< BUG ANCHOR
static int pem_to_der(const char *in, unsigned char **out, size_t *outlen)
{
  char *sep, *start, *end;
  size_t i, j, err;
  size_t len;
  unsigned char *b64;

  /* Jump through the separators in the first line. */
  sep = strstr(in, "-----");
  if(sep == NULL)
    return -1;
  sep = strstr(sep + 1, "-----");
  if(sep == NULL)
    return -1;

  start = sep + 5;

  /* Find beginning of last line separator. */
  end = strstr(start, "-----");
  if(end == NULL)
    return -1;

  len = end - start;
  *out = malloc(len);
  if(!*out)
    return -1;

  b64 = malloc(len + 1);
  if(!b64) {
    free(*out);
    return -1;
  }

  /* Create base64 string without linefeeds. */
  for(i = 0, j = 0; i < len; i++) {
    if(start[i] != '\r' && start[i] != '\n')
      b64[j++] = start[i];
  }
  b64[j] = '\0';

    return -1;
  }

  return 0;
}

static int read_cert(const char *file, unsigned char **out, size_t *outlen)
{
  int fd;
  ssize_t n, len = 0, cap = 512;
  size_t derlen;
  unsigned char buf[cap], *data, *der;

  fd = open(file, 0);
  if(fd < 0)
  }
  data[len] = '\0';

  /*
   * Check if the certificate is in PEM format, and convert it to DER. If this
   * fails, we assume the certificate is in DER format.
   */
  if(pem_to_der((const char *)data, &der, &derlen) == 0) {
    free(data);
    data = der;
    len = derlen;
  }

  *out = data;
  *outlen = len;

  }
}

static int verify_cert(const char *cafile, struct SessionHandle *data,
                       SSLContextRef ctx)
{
  unsigned char *certbuf;
  size_t buflen;
  if(read_cert(cafile, &certbuf, &buflen) < 0) {
    failf(data, "SSL: failed to read or invalid CA certificate");
    return CURLE_SSL_CACERT;
  }

  CFDataRef certdata = CFDataCreate(kCFAllocatorDefault, certbuf, buflen);
  free(certbuf);
  if(!certdata) {
    failf(data, "SSL: failed to allocate array for CA certificate");
    return CURLE_OUT_OF_MEMORY;
  }

  SecCertificateRef cacert = SecCertificateCreateWithData(kCFAllocatorDefault,
   
