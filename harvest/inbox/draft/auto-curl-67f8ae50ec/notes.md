# auto-curl-67f8ae50ec

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8049 (https://github.com/curl/curl/pull/8049)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 478；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -428,6 +428,29 @@ static char *concat_url(const char *base, const char *relurl)
   return newest;
 }
 
+/* scan for byte values < 31 or 127 */
+static bool junkscan(const char *part, unsigned int flags)
+{
+  if(part) {
+    static const char badbytes[]={
+      /* */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
+      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
+      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
+      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
+      0x7f, 0x00 /* null-terminate */
+    };
+    size_t n = strlen(part);
+    size_t nfine = strcspn(part, badbytes);
+    if(nfine != n)
+      /* since we don't know which part is scanned, return a generic error
+         code */
+      return TRUE;
+    if(!(flags & CURLU_ALLOW_SPACE) && strchr(part, ' '))
+      return TRUE;
+  }
+  return FALSE;
+}
+
 /*
  * parse_hostname_login()
  *
@@ -475,7 +498,7 @@ static CURLUcode parse_hostname_login(struct Curl_URL *u,
                                    (h && (h->flags & PROTOPT_URLOPTIONS)) ?
                                    &optionsp:NULL);
   if(ccode) {
-    result = CURLUE_MALFORMED_INPUT;
+    result = CURLUE_BAD_LOGIN;
     goto out;
   }
 
@@ -485,22 +508,38 @@ static CURLUcode parse_hostname_login(struct Curl_URL *u,
       result = CURLUE_USER_NOT_ALLOWED;
       goto out;
     }
-
+    if(junkscan(userp, flags)) {
+      result = CURLUE_BAD_USER;
+      goto out;
+    }
     u->user = userp;
   }
 
-  if(passwdp)
+  if(passwdp) {
+    if(junkscan(passwdp, flags)) {
+      result = CURLUE_BAD_PASSWORD;
+      goto out;
+    }
     u->password = passwdp;
+  }
 
-  if(optionsp)
+  if(optionsp) {
+    if(junkscan(optionsp, flags)) {
+      result = CURLUE_BAD_LOGIN;
+      goto out;
+    }
     u->options = optionsp;
+  }
 
   return CURLUE_OK;
   out:
 
   free(userp);
   free(passwdp);
   free(optionsp);
+  u->user = NULL;
+  u->password = NULL;
+  u->options = NULL;
 
   return result;
 }
@@ -524,19 +563,19 @@ UNITTEST CURLUcode Curl_parse_port(struct Curl_URL *u, char *hostname,
       int zonelen = len;
       if(1 == sscanf(hostname + zonelen, "%*[^]]%c%n", &endbracket, &len)) {
         if(']' != endbracket)
-          return CURLUE_MALFORMED_INPUT;
+          return CURLUE_BAD_IPV6;
         portptr = &hostname[--zonelen + len + 1];
       }
       else
-        return CURLUE_MALFORMED_INPUT;
+        return CURLUE_BAD_IPV6;
     }
     else
-      return CURLUE_MALFORMED_INPUT;
+      return CURLUE_BAD_IPV6;
 
     /* this is a RFC2732-style specified IP-address */
     if(portptr && *portptr) {
       if(*portptr != ':')
-        return CURLUE_MALFORMED_INPUT;
+        return CURLUE_BAD_IPV6;
     }
     else
       portptr = NULL;
@@ -587,29 +626,6 @@ UNITTEST CURLUcode Curl_parse_port(struct Curl_URL *u, char *hostname,
   return CURLUE_OK;
 }
 
-/* scan for byte values < 31 or 127 */
-static bool junkscan(const char *part, unsigned int flags)
-{
-  if(part) {
-    static const char badbytes[]={
-      /* */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
-      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
-      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
-      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
-      0x7f, 0x00 /* null-terminate */
-    };
-    size_t n = strlen(part);
-    size_t nfine = strcspn(part, badbytes);
-    if(nfine != n)
-      /* since we don't know which part is scanned, return a generic error
-         code */
-      return TRUE;
-    if(!(flags & CURLU_ALLOW_SPACE) && strchr(part, ' '))
-      return TRUE;
-  }
-  return FALSE;
-}
-
 static CURLUcode hostname_check(struct Curl_URL *u, char *hostname)
 {
   size_t len;
@@ -621,12 +637,12 @@ static CURLUcode hostname_check(struct Curl_URL *u, char *hostname)
 #endif
     const char *l = "0123456789abcdefABCDEF:.";
     if(hlen < 4) /* '[::]' is the shortest possible valid string */
-      return CURLUE_MALFORMED_INPUT;
+      return CURLUE_BAD_IPV6;
     hostname++;
     hlen -= 2;
 
     if(hostname[h
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-67f8ae50ec` → 本草稿移入 `cases/defect/auto-curl-67f8ae50ec/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
