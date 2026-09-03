// AUTO-DRAFT from curl/curl PR #878
};
#endif
  // <<< BUG ANCHOR

static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
{
#endif
#if defined(USE_WIN32_LDAP)
  TCHAR *host = NULL;
  TCHAR *user = NULL;
  TCHAR *passwd = NULL;
#else
  char *host = NULL;
  char *user = NULL;
  char *passwd = NULL;
#endif

  *done = TRUE; /* unconditionally */
  infof(data, "LDAP local: LDAP Vendor = %s ; LDAP Version = %d\n",

    goto quit;
  }

  if(conn->bits.user_passwd) {
    user = Curl_convert_UTF8_to_tchar(conn->user);
    passwd = Curl_convert_UTF8_to_tchar(conn->passwd);
    if(!user || !passwd) {
      result = CURLE_OUT_OF_MEMORY;

      goto quit;
    }
  }
#else
  host = conn->host.name;

  if(conn->bits.user_passwd) {
    user = conn->user;
    passwd = conn->passwd;
  }
#endif

#ifdef LDAP_OPT_NETWORK_TIMEOUT
  ldap_set_option(NULL, LDAP_OPT_NETWORK_TIMEOUT, &ldap_timeout);
  ldap_set_option(server, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);
#endif

  rc = ldap_simple_bind_s(server, user, passwd);
  if(!ldap_ssl && rc != 0) {
    ldap_proto = LDAP_VERSION2;
    ldap_set_option(server, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);
    rc = ldap_simple_bind_s(server, user, passwd);
  }
  if(rc != 0) {
    failf(data, "LDAP local: ldap_simple_bind_s %s", ldap_err2string(rc));
#endif /* HAVE_LDAP_SSL && CURL_HAS_NOVELL_LDAPSDK */

#if defined(USE_WIN32_LDAP)
  Curl_unicodefree(passwd);
  Curl_unicodefree(user);
  Curl_unicodefree(host);
#endif
