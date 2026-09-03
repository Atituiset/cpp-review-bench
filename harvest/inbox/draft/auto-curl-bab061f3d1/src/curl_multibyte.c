// AUTO-DRAFT from curl/curl PR #13522
#if defined(USE_WIN32_LARGE_FILES) || defined(USE_WIN32_SMALL_FILES)
  // <<< BUG ANCHOR
int curlx_win32_open(const char *filename, int oflag, ...)
{
  int pmode = 0;

#ifdef _UNICODE
  int result = -1;
  wchar_t *filename_w = curlx_convert_UTF8_to_wchar(filename);
#endif


#ifdef _UNICODE
  if(filename_w) {
    result = _wopen(filename_w, oflag, pmode);
    curlx_unicodefree(filename_w);
  }
  else
    errno = EINVAL;
  return result;
#else
  return (_open)(filename, oflag, pmode);
#endif
}

FILE *curlx_win32_fopen(const char *filename, const char *mode)
{
#ifdef _UNICODE
  FILE *result = NULL;
  wchar_t *filename_w = curlx_convert_UTF8_to_wchar(filename);
  wchar_t *mode_w = curlx_convert_UTF8_to_wchar(mode);
  if(filename_w && mode_w)
    result = _wfopen(filename_w, mode_w);
  else
    errno = EINVAL;
  curlx_unicodefree(filename_w);
  curlx_unicodefree(mode_w);
  return result;
#else
  return (fopen)(filename, mode);
#endif
}

int curlx_win32_stat(const char *path, struct_stat *buffer)
{
#ifdef _UNICODE
  int result = -1;
  wchar_t *path_w = curlx_convert_UTF8_to_wchar(path);
  if(path_w) {
#if defined(USE_WIN32_SMALL_FILES)
    result = _wstat(path_w, buffer);
#else
    result = _wstati64(path_w, buffer);
#endif
    curlx_unicodefree(path_w);
  }
  else
    errno = EINVAL;
  return result;
#else
#if defined(USE_WIN32_SMALL_FILES)
  return _stat(path, buffer);
#else
  return _stati64(path, buffer);
#endif
#endif
}

#endif /* USE_WIN32_LARGE_FILES || USE_WIN32_SMALL_FILES */
