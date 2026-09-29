// AUTO-DRAFT from curl/curl PR #8049
return newest;
}
  // <<< BUG ANCHOR
/*
 * parse_hostname_login()
 *
                                   (h && (h->flags & PROTOPT_URLOPTIONS)) ?
                                   &optionsp:NULL);
  if(ccode) {
    result = CURLUE_MALFORMED_INPUT;
    goto out;
  }

      result = CURLUE_USER_NOT_ALLOWED;
      goto out;
    }

    u->user = userp;
  }

  if(passwdp)
    u->password = passwdp;

  if(optionsp)
    u->options = optionsp;

  return CURLUE_OK;
  out:

  free(userp);
  free(passwdp);
  free(optionsp);

  return result;
}
      int zonelen = len;
      if(1 == sscanf(hostname + zonelen, "%*[^]]%c%n", &endbracket, &len)) {
        if(']' != endbracket)
          return CURLUE_MALFORMED_INPUT;
        portptr = &hostname[--zonelen + len + 1];
      }
      else
        return CURLUE_MALFORMED_INPUT;
    }
    else
      return CURLUE_MALFORMED_INPUT;

    /* this is a RFC2732-style specified IP-address */
    if(portptr && *portptr) {
      if(*portptr != ':')
        return CURLUE_MALFORMED_INPUT;
    }
    else
      portptr = NULL;
  return CURLUE_OK;
}

/* scan for byte values < 31 or 127 */
static bool junkscan(const char *part, unsigned int flags)
{
  if(part) {
    static const char badbytes[]={
      /* */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
      0x7f, 0x00 /* null-terminate */
    };
    size_t n = strlen(part);
    size_t nfine = strcspn(part, badbytes);
    if(nfine != n)
      /* since we don't know which part is scanned, return a generic error
         code */
      return TRUE;
    if(!(flags & CURLU_ALLOW_SPACE) && strchr(part, ' '))
      return TRUE;
  }
  return FALSE;
}

static CURLUcode hostname_check(struct Curl_URL *u, char *hostname)
{
  size_t len;
#endif
    const char *l = "0123456789abcdefABCDEF:.";
    if(hlen < 4) /* '[::]' is the shortest possible valid string */
      return CURL
