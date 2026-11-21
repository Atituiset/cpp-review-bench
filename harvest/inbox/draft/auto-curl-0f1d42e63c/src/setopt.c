// AUTO-DRAFT from curl/curl PR #16601
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static CURLcode setstropt_userpwd(char *option, char **userp, char **passwdp)
{
  char *user = NULL;
  char *passwd = NULL;

  DEBUGASSERT(userp);
  DEBUGASSERT(passwdp);

  /* Parse the login details if specified. It not then we treat NULL as a hint
     to clear the existing data */
  if(option) {
    size_t len = strlen(option);
    CURLcode result;
    if(len > CURL_MAX_INPUT_LENGTH)
      return CURLE_BAD_FUNCTION_ARGUMENT;

    result = Curl_parse_login_details(option, len, &user, &passwd, NULL);
    if(result)
      return result;
  }

  free(*userp);
  *userp = user;

  free(*passwdp);
  *passwdp = passwd;

  return CURLE_OK;
}
/* …（同文件无关代码省略）… */
    result = setstropt_userpwd(ptr, &u, &p);

    /* URL decode the components */
    if(!result && u)
      result = Curl_urldecode(u, 0, &data->set.str[STRING_PROXYUSERNAME], NULL,
                              REJECT_ZERO);
    if(!result && p)
      result = Curl_urldecode(p, 0, &data->set.str[STRING_PROXYPASSWORD], NULL,
                              REJECT_ZERO);
    free(u);
    free(p);
  }
