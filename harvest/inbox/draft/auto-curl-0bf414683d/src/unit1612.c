// AUTO-DRAFT from curl/curl PR #15289
unsigned char output[HMAC_MD5_LENGTH];
  unsigned char *testp = output;

  Curl_hmacit(Curl_HMAC_MD5,
              (const unsigned char *) password, strlen(password),
              (const unsigned char *) string1, strlen(string1),
              output);
                "\xd1\x29\x75\x43\x58\xdc\xab\x78\xdf\xcd\x7f\x2b\x29\x31\x13"
                "\x37", HMAC_MD5_LENGTH);

  Curl_hmacit(Curl_HMAC_MD5,
              (const unsigned char *) password, strlen(password),
              (const unsigned char *) string2, strlen(string2),
              output);
