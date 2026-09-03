# auto-curl-361e5aa9e4

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1378 (https://github.com/curl/curl/pull/1378)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1764；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -184,6 +184,68 @@ static curl_off_t VmsSpecialSize(const char *name,
 }
 #endif /* __VMS */
 
+#if defined(HAVE_UTIME) || \
+    (defined(WIN32) && (CURL_SIZEOF_CURL_OFF_T >= 8))
+static void setfiletime(long filetime, const char *filename,
+                        FILE *error_stream)
+{
+  if(filetime >= 0) {
+/* Windows utime() may attempt to adjust our unix gmt 'filetime' by a daylight
+   saving time offset and since it's GMT that is bad behavior. When we have
+   access to a 64-bit type we can bypass utime and set the times directly. */
+#if defined(WIN32) && (CURL_SIZEOF_CURL_OFF_T >= 8)
+    HANDLE hfile;
+
+#if (CURL_SIZEOF_LONG >= 8)
+    /* 910670515199 is the maximum unix filetime that can be used as a
+       Windows FILETIME without overflow: 30827-12-31T23:59:59. */
+    if(filetime > CURL_OFF_T_C(910670515199)) {
+      fprintf(error_stream,
+              "Failed to set filetime %ld on outfile: overflow\n",
+              filetime);
+      return;
+    }
+#endif /* CURL_SIZEOF_LONG >= 8 */
+
+    hfile = CreateFileA(filename, FILE_WRITE_ATTRIBUTES,
+                        (FILE_SHARE_READ | FILE_SHARE_WRITE |
+                         FILE_SHARE_DELETE),
+                        NULL, OPEN_EXISTING, 0, NULL);
+    if(hfile != INVALID_HANDLE_VALUE) {
+      curl_off_t converted = ((curl_off_t)filetime * 10000000) +
+                             CURL_OFF_T_C(116444736000000000);
+      FILETIME ft;
+      ft.dwLowDateTime = (DWORD)(converted & 0xFFFFFFFF);
+      ft.dwHighDateTime = (DWORD)(converted >> 32);
+      if(!SetFileTime(hfile, NULL, &ft, &ft)) {
+        fprintf(error_stream,
+                "Failed to set filetime %ld on outfile: "
+                "SetFileTime failed: GetLastError %u\n",
+                filetime, GetLastError());
+      }
+      CloseHandle(hfile);
+    }
+    else {
+      fprintf(error_stream,
+              "Failed to set filetime %ld on outfile: "
+              "CreateFile failed: GetLastError %u\n",
+              filetime, GetLastError());
+    }
+#elif defined(HAVE_UTIME)
+    struct utimbuf times;
+    times.actime = (time_t)filetime;
+    times.modtime = (time_t)filetime;
+    if(utime(filename, &times)) {
+      fprintf(error_stream,
+              "Failed to set filetime %ld on outfile: errno %d\n",
+              filetime, errno);
+    }
+#endif
+  }
+}
+#endif /* defined(HAVE_UTIME) || \
+          (defined(WIN32) && (CURL_SIZEOF_CURL_OFF_T >= 8)) */
+
 static CURLcode operate_do(struct GlobalConfig *global,
                            struct OperationConfig *config)
 {
@@ -1761,55 +1823,8 @@ static CURLcode operate_do(struct GlobalConfig *global,
           /* Ask libcurl if we got a remote file time */
           long filetime = -1;
           curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
-          if(filetime >= 0) {
-/* Windows utime() may attempt to adjust our unix gmt 'filetime' by a daylight
-   saving time offset and since it's GMT that is bad behavior. When we have
-   access to a 64-bit type we can bypass utime and set the times directly. */
-#if defined(WIN32) && (CURL_SIZEOF_CURL_OFF_T >= 8)
-            /* 910670515199 is the maximum unix filetime that can be used as a
-               Windows FILETIME without overflow: 30827-12-31T23:59:59. */
-            if(filetime <= CURL_OFF_T_C(910670515199)) {
-              HANDLE hfile = CreateFileA(outs.filename, FILE_WRITE_ATTRIBUTES,
-                                         (FILE_SHARE_READ | FILE_SHARE_WRITE |
-                                          FILE_SHARE_DELETE),
-                                         NULL, OPEN_EXISTING, 0, NULL);
-              if(hfile != INVALID_HANDLE_VALUE) {
-                curl_off_t converted = ((curl_off_t)filetime * 10000000) +
-                                       CURL_OFF_T_C(116444736000000000);
-                FILETIME ft;
-                ft.dwLowDateTime = (DWORD)(converted & 0xFFFFFFFF);
-                ft.dwHighDateTime = (DWORD)(converted >>
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-361e5aa9e4` → 本草稿移入 `cases/defect/auto-curl-361e5aa9e4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
