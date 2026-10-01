// AUTO-DRAFT from curl/curl PR #2149
sshc->actualcode = CURLE_QUOTE_ERROR;
        break;
      }
      else {  // <<< BUG ANCHOR
        char *tmp = aprintf("statvfs:\n"
                            "f_bsize: %llu\n" "f_frsize: %llu\n"
                            "f_blocks: %llu\n" "f_bfree: %llu\n"
