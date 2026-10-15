// AUTO-DRAFT from curl/curl PR #2029
* after call get_parm_word, str either point to string end
 * or point to any of end chars.
 */
static char *get_param_word(char **str, char **end_pos)
{
  char *ptr = *str;
  char *word_begin = NULL;
  char *ptr2;
  char *escape = NULL;
  const char *end_chars = ";,";
  // <<< BUG ANCHOR
  /* the first non-space char is here */
  word_begin = ptr;
          while(ptr < *end_pos);
          *end_pos = ptr2;
        }
        while(*ptr && NULL == strchr(end_chars, *ptr))
          ++ptr;
        *str = ptr;
        return word_begin + 1;
    ptr = word_begin;
  }

  while(*ptr && NULL == strchr(end_chars, *ptr))
    ++ptr;
  *str = *end_pos = ptr;
  return word_begin;
  /* NOTREACHED */
}

static int get_param_part(struct OperationConfig *config, char **str,
                          char **pdata, char **ptype, char **pfilename,
                          char **pencoder, struct curl_slist **pheaders)
{
  char *p = *str;
  char *type = NULL;
  while(ISSPACE(*p))
    p++;
  tp = p;
  *pdata = get_param_word(&p, &endpos);
  /* If not quoted, strip trailing spaces. */
  if(*pdata == tp)
    while(endpos > *pdata && ISSPACE(endpos[-1]))
      }

      /* now point beyond the content-type specifier */
      endpos = type + strlen(type_major) + strlen(type_minor) + 1;
      for(p = endpos; ISSPACE(*p); p++)
        ;
      while(*p && *p != ';' && *p != ',')
        p++;
      endct = p;
      sep = *p;
    }
    else if(checkprefix("filename=", p)) {
      for(p += 9; ISSPACE(*p); p++)
        ;
      tp = p;
      filename = get_param_word(&p, &endpos);
      /* If not quoted, strip trailing spaces. */
      if(filename == tp)
        while(endpos > filename && ISSPACE(endpos[-1]))
          p++;
        } while(ISSPACE(*p));
        tp = p;
        hdrfile = get_param_word(&p, &endpos);
        /* If not quoted, strip trailing spaces. */
        if(hdrfile == tp)
          while(endpos > hdrfile && ISSPACE(endpos[-1]))
        while(ISSPACE(*p))
          p++;
        tp = p;
        hdr 
