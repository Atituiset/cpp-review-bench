// AUTO-DRAFT from curl/curl PR #8835
conn->ssl_config.verifypeer = data->set.ssl.primary.verifypeer;
  conn->ssl_config.verifyhost = data->set.ssl.primary.verifyhost;
  conn->ssl_config.ssl_options = data->set.ssl.primary.ssl_options;
#ifdef USE_TLS_SRP
#endif
#ifndef CURL_DISABLE_PROXY
  conn->proxy_ssl_config.verifystatus =
    data->set.proxy_ssl.primary.verifystatus;
  conn->proxy_ssl_config.verifypeer = data->set.proxy_ssl.primary.verifypeer;
  conn->proxy_ssl_config.verifyhost = data->set.proxy_ssl.primary.verifyhost;
  conn->proxy_ssl_config.ssl_options = data->set.proxy_ssl.primary.ssl_options;
#ifdef USE_TLS_SRP
#endif
#endif
  conn->ip_version = data->set.ipver;
  conn->bits.connect_only = data->set.connect_only;
