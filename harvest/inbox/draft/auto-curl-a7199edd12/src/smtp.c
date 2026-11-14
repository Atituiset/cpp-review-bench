// AUTO-DRAFT from curl/curl PR #23604
#define SMTP_EOB_FIND_LEN 3
/* …（同文件无关代码省略）… */
      for(i = start = 0; i < nread; ++i) {
        if(ctx->n_eob >= SMTP_EOB_FIND_LEN) {
          /* matched the EOB prefix and seeing additional char, add '.' */
          result = Curl_bufq_cwrite(&ctx->buf, buf + start, i - start, &n);
          if(result)
            return result;
