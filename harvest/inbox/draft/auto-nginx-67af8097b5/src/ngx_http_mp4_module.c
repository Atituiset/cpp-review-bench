// AUTO-DRAFT from nginx/nginx PR #345
start_sample -= n;
  // <<< BUG ANCHOR
        prev_samples = samples;
        chunk = next_chunk;
        samples = ngx_mp4_get_32value(entry->samples);
        id = ngx_mp4_get_32value(entry->id);

    next_chunk = trak->chunks + 1;

    ngx_log_debug4(NGX_LOG_DEBUG_HTTP, mp4->file.log, 0,
                   "sample:%uD, chunk:%uD, chunks:%uD, samples:%uD",
                   start_sample, chunk, next_chunk - chunk, samples);
        return NGX_ERROR;
    }

    target_chunk = chunk - 1;
    target_chunk += start_sample / samples;
    chunk_samples = start_sample % samples;
