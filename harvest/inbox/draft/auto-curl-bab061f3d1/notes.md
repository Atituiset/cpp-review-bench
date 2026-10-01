# auto-curl-bab061f3d1

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #13522 (https://github.com/curl/curl/pull/13522)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 96；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -88,12 +88,170 @@ char *curlx_convert_wchar_to_UTF8(const wchar_t *str_w)
 
 #if defined(USE_WIN32_LARGE_FILES) || defined(USE_WIN32_SMALL_FILES)
 
+/* declare GetFullPathNameW for mingw-w64 UWP builds targeting old windows */
+#if defined(CURL_WINDOWS_UWP) && defined(__MINGW32__) && \
+  (_WIN32_WINNT < _WIN32_WINNT_WIN10)
+WINBASEAPI DWORD WINAPI GetFullPathNameW(LPCWSTR, DWORD, LPWSTR, LPWSTR *);
+#endif
+
+/* Fix excessive paths (paths that exceed MAX_PATH length of 260).
+ *
+ * This is a helper function to fix paths that would exceed the MAX_PATH
+ * limitation check done by Windows APIs. It does so by normalizing the passed
+ * in filename or path 'in' to its full canonical path, and if that path is
+ * longer than MAX_PATH then setting 'out' to "\\?\" prefix + that full path.
+ *
+ * For example 'in' filename255chars in current directory C:\foo\bar is
+ * fixed as \\?\C:\foo\bar\filename255chars for 'out' which will tell Windows
+ * it is ok to access that filename even though the actual full path is longer
+ * than 260 chars.
+ *
+ * For non-Unicode builds this function may fail sometimes because only the
+ * Unicode versions of some Windows API functions can access paths longer than
+ * MAX_PATH, for example GetFullPathNameW which is used in this function. When
+ * the full path is then converted from Unicode to multibyte that fails if any
+ * directories in the path contain characters not in the current codepage.
+ */
+static bool fix_excessive_path(const TCHAR *in, TCHAR **out)
+{
+  size_t needed, count;
+  const wchar_t *in_w;
+  wchar_t *fbuf = NULL;
+
+  /* MS documented "approximate" limit for the maximum path length */
+  const size_t max_path_len = 32767;
+
+#ifndef _UNICODE
+  wchar_t *ibuf = NULL;
+  char *obuf = NULL;
+#endif
+
+  *out = NULL;
+
+  /* skip paths already normalized */
+  if(!_tcsncmp(in, _T("\\\\?\\"), 4))
+    goto cleanup;
+
+#ifndef _UNICODE
+  /* convert multibyte input to unicode */
+  needed = mbstowcs(NULL, in, 0);
+  if(needed == (size_t)-1 || needed >= max_path_len)
+    goto cleanup;
+  ++needed; /* for NUL */
+  ibuf = malloc(needed * sizeof(wchar_t));
+  if(!ibuf)
+    goto cleanup;
+  count = mbstowcs(ibuf, in, needed);
+  if(count == (size_t)-1 || count >= needed)
+    goto cleanup;
+  in_w = ibuf;
+#else
+  in_w = in;
+#endif
+
+  /* GetFullPathNameW returns the normalized full path in unicode. It converts
+     forward slashes to backslashes, processes .. to remove directory segments,
+     etc. Unlike GetFullPathNameA it can process paths that exceed MAX_PATH. */
+  needed = (size_t)GetFullPathNameW(in_w, 0, NULL, NULL);
+  if(!needed || needed > max_path_len)
+    goto cleanup;
+  /* skip paths that are not excessive and do not need modification */
+  if(needed <= MAX_PATH)
+    goto cleanup;
+  fbuf = malloc(needed * sizeof(wchar_t));
+  if(!fbuf)
+    goto cleanup;
+  count = (size_t)GetFullPathNameW(in_w, (DWORD)needed, fbuf, NULL);
+  if(!count || count >= needed)
+    goto cleanup;
+
+  /* prepend \\?\ or \\?\UNC\ to the excessively long path.
+   *
+   * c:\longpath            --->    \\?\c:\longpath
+   * \\.\c:\longpath        --->    \\?\c:\longpath
+   * \\?\c:\longpath        --->    \\?\c:\longpath  (unchanged)
+   * \\server\c$\longpath   --->    \\?\UNC\server\c$\longpath
+   *
+   * https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
+   */
+  if(!wcsncmp(fbuf, L"\\\\?\\", 4))
+    ; /* do nothing */
+  else if(!wcsncmp(fbuf, L"\\\\.\\", 4))
+    fbuf[2] = '?';
+  else if(!wcsncmp(fbuf, L"\\\\.", 3) || !wcsncmp(fbuf, L"\\\\?", 3)) {
+    /* Unexpected, not UNC. The formatting doc doesn't allow this AFAICT. */
+    goto cleanup;
+  }
+  else {
+    wchar_t *temp;
+
+    if(!wcsncmp(fbuf, L"\\\\", 2)) {
+      /* "\\?\UNC\" + full path without "\\" + null */
+      needed = 8 + (count - 2) + 1;
+      if(needed > max_path_len)
+        goto cleanup;
+
+      temp = malloc(needed * sizeof(wchar_t));
+      if(!temp)
+        goto cleanup;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-bab061f3d1` → 本草稿移入 `cases/defect/auto-curl-bab061f3d1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
