// AUTO-DRAFT from curl/curl PR #8035
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "tool_setup.h"
  // <<< BUG ANCHOR
#ifdef HAVE_PWD_H
#  undef __NO_NET_API /* required for building for AmigaOS */
#  include <pwd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <curl/mprintf.h>

#include "tool_homedir.h"

#include "memdebug.h" /* keep this as LAST include */

static char *GetEnv(const char *variable)
{
  char *dupe, *env;

  env = curl_getenv(variable);
  if(!env)
    return NULL;

  dupe = strdup(env);
  curl_free(env);
  return dupe;
}

/* return the home directory of the current user as an allocated string */

/*
 * The original logic found a home dir to use (by checking a range of
 * environment variables and last using getpwuid) and returned that for the
 * parent to use.
 *
 * With the XDG_CONFIG_HOME support (added much later than the other), this
 * variable is treated differently in order to not ruin existing installations
 * even if this environment variable is set. If this va
