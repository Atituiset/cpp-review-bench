// AUTO-DRAFT from curl/curl PR #0dc22b690dd8dba4048d494f09a50122dd7c0dd4
uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  CURLcode result;
  // <<< BUG ANCHOR
  DEBUGASSERT(buf);

    switch(val8) {
    case CURL_SPACK_ALPN:
      result = spack_decstr16(&s->alpn, &buf, end);
      if(result)
        goto out;
      s->ietf_tls_id = val16;
      break;
    case CURL_SPACK_QUICTP: {
      result = spack_decdata16(&pval8, &s->quic_tp_len, &buf, end);
      if(result)
        goto out;
      s->quic_tp = pval8;
      break;
    }
    case CURL_SPACK_TICKET: {
      result = spack_decdata16(&pval8, &s->sdata_len, &buf, end);
      if(result)
        goto out;
      s->sdata = pval8;
      break;
    }
    case CURL_SPACK_VALID_UNTIL:
