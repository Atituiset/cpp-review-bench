// AUTO-DRAFT from curl/curl PR #bc440a89d47aa8f3a5d02b01985fd7f6fb7e7e0a
return cursize;
}

static curl_off_t encoder_base64_size(curl_mimepart *part)
{
  curl_off_t size = part->datasize;

  if(size <= 0)
    return size;    /* Unknown size or no data. */

  /* Compute base64 character count. */
  size = 4 * (1 + ((size - 1) / 3));
