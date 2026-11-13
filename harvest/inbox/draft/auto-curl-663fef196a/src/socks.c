// AUTO-DRAFT from curl/curl PR #20813

  if(result || !dns) {
    failf(data, "Failed to resolve \"%s\" for SOCKS4 connect.", sx->hostname);
    return CURLPX_RESOLVE_HOST;
  }

/* …（同文件无关代码省略）… */
      return CURLPX_SEND_REQUEST;
  }
  else {
    failf(data, "SOCKS4 connection to %s not supported", sx->hostname);
    return CURLPX_RESOLVE_HOST;
  }
