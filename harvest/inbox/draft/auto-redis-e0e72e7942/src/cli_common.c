// AUTO-DRAFT from redis/redis PR #15255
/* Extract user info. */
    if ((userinfo = strchr(curr,'@'))) {
        if ((username = strchr(curr, ':')) && username < userinfo) {
            connInfo->user = percentDecode(curr, username - curr);  // <<< BUG ANCHOR
            curr = username + 1;
        }

        connInfo->auth = percentDecode(curr, userinfo - curr);
        curr = userinfo + 1;
    }
    if (curr == end) return;
