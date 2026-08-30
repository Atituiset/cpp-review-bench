// AUTO-DRAFT from curl/curl PR #52
return CURLE_OK;
}

static CURLcode pop3_state_apop(struct connectdata *conn)
{
  CURLcode result = CURLE_OK;

  return result;
}

static CURLcode pop3_authenticate(struct connectdata *conn)
{
    /* Check supported authentication types by decreasing order of security */
    if(conn->proto.pop3c.authtypes & POP3_TYPE_SASL)
      result = pop3_authenticate(conn);
    else if(conn->proto.pop3c.authtypes & POP3_TYPE_APOP)
      result = pop3_state_apop(conn);
    else if(conn->proto.pop3c.authtypes & POP3_TYPE_CLEARTEXT)
      result = pop3_state_user(conn);
    else {
