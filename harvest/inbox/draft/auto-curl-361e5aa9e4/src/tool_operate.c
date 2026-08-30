// AUTO-DRAFT from curl/curl PR #1378
}
#endif /* __VMS */
  // <<< BUG ANCHOR
static CURLcode operate_do(struct GlobalConfig *global,
                           struct OperationConfig *config)
{
          /* Ask libcurl if we got a remote file time */
          long filetime = -1;
          curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
          if(filetime >= 0) {
/* Windows utime() may attempt to adjust our unix gmt 'filetime' by a daylight
   saving time offset and since it's GMT that is bad behavior. When we have
   access to a 64-bit type we can bypass utime and set the times directly. */
#if defined(WIN32) && (CURL_SIZEOF_CURL_OFF_T >= 8)
            /* 910670515199 is the maximum unix filetime that can be used as a
               Windows FILETIME without overflow: 30827-12-31T23:59:59. */
            if(filetime <= CURL_OFF_T_C(910670515199)) {
              HANDLE hfile = CreateFileA(outs.filename, FILE_WRITE_ATTRIBUTES,
                                         (FILE_SHARE_READ | FILE_SHARE_WRITE |
                                          FILE_SHARE_DELETE),
                                         NULL, OPEN_EXISTING, 0, NULL);
              if(hfile != INVALID_HANDLE_VALUE) {
                curl_off_t converted = ((curl_off_t)filetime * 10000000) +
                                       CURL_OFF_T_C(116444736000000000);
                FILETIME ft;
                ft.dwLowDateTime = (DWORD)(converted & 0xFFFFFFFF);
                ft.dwHighDateTime = (DWORD)(converted >> 32);
                if(!SetFileTime(hfile, NULL, &ft, &ft)) {
                  fprintf(config->global->errors,
                          "Failed to set filetime %ld on outfile: "
                          "SetFileTime failed: GetLastError %u\n",
                          filetime, GetLastError());
                }
                CloseHandle(hfile);
              }
              else {
                fprintf(config->global->errors,
                        "Failed to set filetime %ld on outfile: "
                        "CreateFi
