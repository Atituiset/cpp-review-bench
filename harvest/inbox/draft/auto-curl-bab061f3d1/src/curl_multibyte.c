// AUTO-DRAFT from curl/curl PR #13522
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
wchar_t *curlx_convert_UTF8_to_wchar(const char *str_utf8)
{
  wchar_t *str_w = NULL;

  if(str_utf8) {
    int str_w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        str_utf8, -1, NULL, 0);
    if(str_w_len > 0) {
      str_w = malloc(str_w_len * sizeof(wchar_t));
      if(str_w) {
        if(MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, str_w,
                               str_w_len) == 0) {
          free(str_w);
          return NULL;
        }
      }
    }
  }

  return str_w;
}
/* …（同文件无关代码省略）… */

#if defined(USE_WIN32_LARGE_FILES) || defined(USE_WIN32_SMALL_FILES)

int curlx_win32_open(const char *filename, int oflag, ...)
{
  int pmode = 0;

#ifdef _UNICODE
  int result = -1;
  wchar_t *filename_w = curlx_convert_UTF8_to_wchar(filename);
#endif

  va_list param;
  va_start(param, oflag);
  if(oflag & O_CREAT)
    pmode = va_arg(param, int);
  va_end(param);

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
