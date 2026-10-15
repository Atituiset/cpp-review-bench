# auto-curl-25cac6874b

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #115 (https://github.com/curl/curl/pull/115)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1539；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1527,43 +1527,42 @@ static CURLcode darwinssl_connect_step1(struct connectdata *conn,
   return CURLE_OK;
 }
 
-static int pem_to_der(const char *in, unsigned char **out, size_t *outlen)
+static long pem_to_der(const char *in, unsigned char **out, size_t *outlen)
 {
-  char *sep, *start, *end;
+  char *sep_start, *sep_end, *cert_start, *cert_end;
   size_t i, j, err;
   size_t len;
   unsigned char *b64;
 
-  /* Jump through the separators in the first line. */
-  sep = strstr(in, "-----");
-  if(sep == NULL)
-    return -1;
-  sep = strstr(sep + 1, "-----");
-  if(sep == NULL)
+  /* Jump through the separators at the beginning of the certificate. */
+  sep_start = strstr(in, "-----");
+  if(sep_start == NULL)
+    return 0;
+  cert_start = strstr(sep_start + 1, "-----");
+  if(cert_start == NULL)
     return -1;
 
-  start = sep + 5;
+  cert_start += 5;
 
-  /* Find beginning of last line separator. */
-  end = strstr(start, "-----");
-  if(end == NULL)
+  /* Find separator after the end of the certificate. */
+  cert_end = strstr(cert_start, "-----");
+  if(cert_end == NULL)
     return -1;
 
-  len = end - start;
-  *out = malloc(len);
-  if(!*out)
+  sep_end = strstr(cert_end + 1, "-----");
+  if(sep_end == NULL)
     return -1;
+  sep_end += 5;
 
+  len = cert_end - cert_start;
   b64 = malloc(len + 1);
-  if(!b64) {
-    free(*out);
+  if(!b64)
     return -1;
-  }
 
   /* Create base64 string without linefeeds. */
   for(i = 0, j = 0; i < len; i++) {
-    if(start[i] != '\r' && start[i] != '\n')
-      b64[j++] = start[i];
+    if(cert_start[i] != '\r' && cert_start[i] != '\n')
+      b64[j++] = cert_start[i];
   }
   b64[j] = '\0';
 
@@ -1574,15 +1573,14 @@ static int pem_to_der(const char *in, unsigned char **out, size_t *outlen)
     return -1;
   }
 
-  return 0;
+  return sep_end - in;
 }
 
 static int read_cert(const char *file, unsigned char **out, size_t *outlen)
 {
   int fd;
   ssize_t n, len = 0, cap = 512;
-  size_t derlen;
-  unsigned char buf[cap], *data, *der;
+  unsigned char buf[cap], *data;
 
   fd = open(file, 0);
   if(fd < 0)
@@ -1620,16 +1618,6 @@ static int read_cert(const char *file, unsigned char **out, size_t *outlen)
   }
   data[len] = '\0';
 
-  /*
-   * Check if the certificate is in PEM format, and convert it to DER. If this
-   * fails, we assume the certificate is in DER format.
-   */
-  if(pem_to_der((const char *)data, &der, &derlen) == 0) {
-    free(data);
-    data = der;
-    len = derlen;
-  }
-
   *out = data;
   *outlen = len;
 
@@ -1665,51 +1653,134 @@ static int sslerr_to_curlerr(struct SessionHandle *data, int err)
   }
 }
 
+static int append_cert_to_array(struct SessionHandle *data,
+                                unsigned char *buf, size_t buflen,
+                                CFMutableArrayRef array)
+{
+    CFDataRef certdata = CFDataCreate(kCFAllocatorDefault, buf, buflen);
+    if(!certdata) {
+      failf(data, "SSL: failed to allocate array for CA certificate");
+      return CURLE_OUT_OF_MEMORY;
+    }
+
+    SecCertificateRef cacert =
+      SecCertificateCreateWithData(kCFAllocatorDefault, certdata);
+    CFRelease(certdata);
+    if(!cacert) {
+      failf(data, "SSL: failed to create SecCertificate from CA certificate");
+      return CURLE_SSL_CACERT;
+    }
+
+    /* Check if cacert is valid. */
+    SecKeyRef key;
+    OSStatus ret = SecCertificateCopyPublicKey(cacert, &key);
+    if(ret != noErr) {
+      CFRelease(cacert);
+      failf(data, "SSL: invalid CA certificate");
+      return CURLE_SSL_CACERT;
+    }
+    CFRelease(key);
+
+    CFArrayAppendValue(array, cacert);
+    CFRelease(cacert);
+
+    return CURLE_OK;
+}
+
 static int verify_cert(const char *cafile, struct SessionHandle *data,
                        SSLContextRef ctx)
 {
-  unsigned char *certbuf;
-  size_t buflen;
+  int n = 0, rc;
+  long res;
+  unsigned char *certbuf, *der;
+  size_t buflen, derlen, offset = 0;
+
   if(read_cert(cafile, &certbuf, &buflen) < 0) {
     failf(d
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-25cac6874b` → 本草稿移入 `cases/defect/auto-curl-25cac6874b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
