// AUTO-DRAFT from curl/curl PR #8049
return "An invalid 'part' argument was passed as argument";
  // <<< BUG ANCHOR
  case CURLUE_MALFORMED_INPUT:
    return "A malformed input was passed to a URL API function";

  case CURLUE_BAD_PORT_NUMBER:
    return "The port number was not a decimal number between 0 and 65535";

  case CURLUE_UNSUPPORTED_SCHEME:
    return "This libcurl build doesn't support the given URL scheme";
    return "An unknown part ID was passed to a URL API function";

  case CURLUE_NO_SCHEME:
    return "There is no scheme part in the URL";

  case CURLUE_NO_USER:
    return "There is no user part in the URL";

  case CURLUE_NO_PASSWORD:
    return "There is no password part in the URL";

  case CURLUE_NO_OPTIONS:
    return "There is no options part in the URL";

  case CURLUE_NO_HOST:
    return "There is no host part in the URL";

  case CURLUE_NO_PORT:
    return "There is no port part in the URL";

  case CURLUE_NO_QUERY:
    return "There is no query part in the URL";

  case CURLUE_NO_FRAGMENT:
    return "There is no fragment part in the URL";

  case CURLUE_LAST:
    break;
