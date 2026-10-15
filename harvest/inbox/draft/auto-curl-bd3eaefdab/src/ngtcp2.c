// AUTO-DRAFT from curl/curl PR #4735
if(level == NGTCP2_CRYPTO_LEVEL_APP) {
    if(init_ngh3_conn(qs) != CURLE_OK)
      return 0;
  // <<< BUG ANCHOR
    /* malloc an area big enough for both secrets */
    qs->rx_secret = malloc(secretlen * 2);
    if(!qs->rx_secret)
      return 0;
    memcpy(qs->rx_secret, rx_secret, secretlen);
    memcpy(&qs->rx_secret[secretlen], tx_secret, secretlen);
    qs->tx_secret = &qs->rx_secret[secretlen];
    qs->rx_secretlen = secretlen;
  }

  return 1;
  return 0;
}

static int cb_update_key(ngtcp2_conn *tconn, uint8_t *rx_key,
                         uint8_t *rx_iv, uint8_t *tx_key,
                         uint8_t *tx_iv, void *user_data)
{
  struct quicsocket *qs = (struct quicsocket *)user_data;
  uint8_t rx_secret[64];
  uint8_t tx_secret[64];

  if(ngtcp2_crypto_update_key(tconn, rx_secret, tx_secret,
                              rx_key, rx_iv, tx_key, tx_iv, qs->rx_secret,
                              qs->tx_secret, qs->rx_secretlen) != 0)
    return NGTCP2_ERR_CALLBACK_FAILURE;

  /* store the updated secrets */
  memcpy(qs->rx_secret, rx_secret, qs->rx_secretlen);
  memcpy(qs->tx_secret, tx_secret, qs->rx_secretlen);
  return 0;
}

static ngtcp2_conn_callbacks ng_callbacks = {
  cb_initial,
  NULL, /* recv_client_initial */
  NULL, /* rand  */
  cb_get_new_connection_id,
  NULL, /* remove_connection_id */
  cb_update_key, /* update_key */
  NULL, /* path_validation */
  NULL, /* select_preferred_addr */
  cb_stream_reset,
  int i;
  struct quicsocket *qs = &conn->hequic[0];
  (void)dead_connection;
  free(qs->rx_secret);
  if(qs->ssl)
    SSL_free(qs->ssl);
  for(i = 0; i < 3; i++)
