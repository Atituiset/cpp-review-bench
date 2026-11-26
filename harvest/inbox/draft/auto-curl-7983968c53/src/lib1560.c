// AUTO-DRAFT from curl/curl PR #8049
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct testcase {
  const char *in;
  const char *out;
  unsigned int urlflags;
  unsigned int getflags;
  CURLUcode ucode;
};
/* …（同文件无关代码省略）… */
};

static const struct testcase get_parts_list[] ={
  {"https://user:password@example.net/get?this=and what", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://user:password@example.net/ge t?this=and-what", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://user:pass word@example.net/get?this=and-what", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://u ser:password@example.net/get?this=and-what", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  /* no space allowed in scheme */
  {"htt ps://user:password@example.net/get?this=and-what", "",
   CURLU_NON_SUPPORT_SCHEME|CURLU_ALLOW_SPACE, 0, CURLUE_MALFORMED_INPUT},
  {"https://user:password@example.net/get?this=and what",
   "https | user | password | [13] | example.net | [15] | /get | "
   "this=and what | [17]",
/* …（同文件无关代码省略）… */
   "com/color/?green#no-red",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_OK },
  {"http://[ab.be:1]/x", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"http://[ab.be]/x", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  /* URL without host name */
  {"http://a:b@/x", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_NO_HOST},
/* …（同文件无关代码省略）… */
   0, 0, CURLUE_OK},
  {"file://hello.html",
   "",
   0, 0, CURLUE_MALFORMED_INPUT},
  {"http://HO0_-st/",
   "http | [11] | [12] | [13] | HO0_-st | [15] | / | [16] | [17]",
   0, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   0, 0, CURLUE_OK},
  {"file:/",
   "file | [11] | [12] | [13] | [14] | [15] | | [16] | [17]",
   0, 0, CURLUE_MALFORMED_INPUT},
  {"file://127.0.0.1/hello.html",
   "file | [11] | [12] | [13] | [14] | [15] | /hello.html | [16] | [17]",
   0, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   "https | [11] | [12] | [13] | 127abc.com | [15] | / | [16] | [17]",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
  {"https:// example.com?check", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://e x a m p l e.com?check", "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://example.com?check",
   "https | [11] | [12] | [13] | example.com | [15] | / | check | [17]",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
  {"http:////user:password@example.com:1234/path/html?query=name#anchor",
   "",
   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {NULL, NULL, 0, 0, CURLUE_OK},
};

/* …（同文件无关代码省略）… */
  {"https://h%c", "https://h%25c/", 0, 0, CURLUE_OK},
  {"https://%%%%%%", "https://%25%25%25%25%25%25/", 0, 0, CURLUE_OK},
  {"https://%41", "https://A/", 0, 0, CURLUE_OK},
  {"https://%20", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"https://%41%0d", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"https://%25", "https://%25/", 0, 0, CURLUE_OK},
  {"https://_%c0_", "https://_\xC0_/", 0, 0, CURLUE_OK},
  {"https://_%c0_", "https://_%C0_/", 0, CURLU_URLENCODE, CURLUE_OK},
/* …（同文件无关代码省略）… */
  {"https://+127.0.0.1", "https://%2B127.0.0.1/", 0, CURLU_URLENCODE,
   CURLUE_OK},
  {"https://127.-0.0.1", "https://127.-0.0.1/", 0, 0, CURLUE_OK},
  {"https://127.0. 1", "https://127.0.0.1/", 0, 0, CURLUE_MALFORMED_INPUT},
  {"https://1.0x1000000", "https://1.0x1000000/", 0, 0, CURLUE_OK},
  {"https://1.2.3.256", "https://1.2.3.256/", 0, 0, CURLUE_OK},
  {"https://1.2.3.4.5", "https://1.2.3.4.5/", 0, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
  /* 41 bytes scheme is not allowed */
  {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA://hostname/path",
   "",
   CURLU_NON_SUPPORT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"https://[fe80::20c:29ff:fe9c:409b%]:1234",
   "",
   0, 0, CURLUE_MALFORMED_INPUT},
  {"https://[fe80::20c:29ff:fe9c:409b%25]:1234",
   "https://[fe80::20c:29ff:fe9c:409b%2525]:1234/",
   0, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   CURLU_GUESS_SCHEME, 0, CURLUE_OK},
  {"HTTP://test/", "http://test/", 0, 0, CURLUE_OK},
  {"http://HO0_-st..~./", "http://HO0_-st..~./", 0, 0, CURLUE_OK},
  {"http:/@example.com: 123/", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"http:/@example.com:123 /", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"http:/@example.com:123a/", "", 0, 0, CURLUE_BAD_PORT_NUMBER},
  {"http://host/file\r", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"http://host/file\n\x03", "", 0, 0, CURLUE_MALFORMED_INPUT},
  {"htt\x02://host/file", "",
   CURLU_NON_SUPPORT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {" http://host/file", "", 0, 0, CURLUE_MALFORMED_INPUT},
  /* here the password ends at the semicolon and options is 'word' */
  {"imap://user:pass;word@host/file",
   "imap://user:pass;word@host/file",
/* …（同文件无关代码省略）… */
   0, 0, CURLUE_OK},
  {"file:./",
   "file://",
   0, 0, CURLUE_MALFORMED_INPUT},
  {"http://example.com/hello/../here",
   "http://example.com/hello/../here",
   CURLU_PATH_AS_IS, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
  {"example.com/path/html",
   "",
   0, 0, CURLUE_MALFORMED_INPUT},
  {"http://user:password@example.com:1234/path/html?query=name#anchor",
   "http://user:password@example.com:1234/path/html?query=name#anchor",
   0, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   CURLU_NON_SUPPORT_SCHEME, 0, CURLUE_OK},
  {"custom-scheme://?expected=test-bad",
   "",
   CURLU_NON_SUPPORT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
  {"custom-scheme://?expected=test-new-good",
   "custom-scheme:///?expected=test-new-good",
   CURLU_NON_SUPPORT_SCHEME | CURLU_NO_AUTHORITY, 0, CURLUE_OK},
/* …（同文件无关代码省略）… */
   /* Set a 41 bytes scheme. That's too long so the old scheme remains set. */
   "scheme=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbc,",
   "https://example.com/",
   0, CURLU_NON_SUPPORT_SCHEME, CURLUE_OK, CURLUE_MALFORMED_INPUT},
  {"https://example.com/",
   /* set a 40 bytes scheme */
   "scheme=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,",
/* …（同文件无关代码省略）… */
  {"https://host:1234/",
   "port=56 78,",
   
