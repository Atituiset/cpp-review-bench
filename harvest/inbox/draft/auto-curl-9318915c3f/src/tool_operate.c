// AUTO-DRAFT from curl/curl PR #8035
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "tool_filetime.h"
#include "tool_getparam.h"
#include "tool_helpers.h"
#include "tool_homedir.h"
#include "tool_libinfo.h"
#include "tool_main.h"
#include "tool_msgs.h"
/* …（同文件无关代码省略）… */

        if((use_proto & (CURLPROTO_SCP|CURLPROTO_SFTP)) &&
           !config->insecure_ok) {
          char *home = homedir(NULL);
          if(home) {
            char *file = aprintf("%s/.ssh/known_hosts", home);
            if(file) {
              /* new in curl 7.19.6 */
              result = res_setopt_str(curl, CURLOPT_SSH_KNOWNHOSTS, file);
              curl_free(file);
              if(result == CURLE_UNKNOWN_OPTION)
                /* libssh2 version older than 1.1.1 */
                result = CURLE_OK;
            }
            Curl_safefree(home);
            if(result)
              break;
          }
          else
            warnf(global, "No home dir, couldn't find known_hosts file!");
        }

        if(config->no_body || config->remote_time) {
