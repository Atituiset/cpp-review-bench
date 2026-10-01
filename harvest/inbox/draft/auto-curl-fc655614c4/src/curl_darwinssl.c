// AUTO-DRAFT from curl/curl PR #93
}
#endif /* CURL_BUILD_MAC_10_6 || CURL_BUILD_IOS */
  // <<< BUG ANCHOR
  /* If this is a domain name and not an IP address, then configure SNI.
   * Also: the verifyhost setting influences SNI usage */
  /* If this is a domain name and not an IP address, then configure SNI: */
  if((0 == Curl_inet_pton(AF_INET, conn->host.name, &addr)) &&
#ifdef ENABLE_IPV6
     (0 == Curl_inet_pton(AF_INET6, conn->host.name, &addr)) &&
#endif
     data->set.ssl.verifyhost) {
    err = SSLSetPeerDomainName(connssl->ssl_ctx, conn->host.name,
                               strlen(conn->host.name));
    if(err != noErr) {
      infof(data, "WARNING: SSL: SSLSetPeerDomainName() failed: OSStatus %d\n",
            err);
    }
  }

  /* Disable cipher suites that ST supports but are not safe. These ciphers
