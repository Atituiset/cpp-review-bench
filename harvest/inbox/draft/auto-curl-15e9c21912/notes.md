# auto-curl-15e9c21912

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #878 (https://github.com/curl/curl/pull/878)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 246；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -181,6 +181,81 @@ const struct Curl_handler Curl_handler_ldaps = {
 };
 #endif
 
+#if defined(USE_WIN32_LDAP)
+
+#if defined(USE_WINDOWS_SSPI)
+static int ldap_win_bind_auth(LDAP *server, const char *user,
+                              const char *passwd, unsigned long authflags)
+{
+  ULONG method = 0;
+  SEC_WINNT_AUTH_IDENTITY cred = { 0, };
+  int rc = LDAP_AUTH_METHOD_NOT_SUPPORTED;
+
+#if defined(USE_SPNEGO)
+  if(authflags & CURLAUTH_NEGOTIATE) {
+    method = LDAP_AUTH_NEGOTIATE;
+  }
+  else
+#endif
+#if defined(USE_NTLM)
+  if(authflags & CURLAUTH_NTLM) {
+    method = LDAP_AUTH_NTLM;
+  }
+  else
+#endif
+#if !defined(CURL_DISABLE_CRYPTO_AUTH)
+  if(authflags & CURLAUTH_DIGEST) {
+    method = LDAP_AUTH_DIGEST;
+  }
+  else
+#endif
+  {
+    /* required anyway if one of upper preprocessor definitions enabled */
+  }
+
+  if(method && user && passwd) {
+    rc = Curl_create_sspi_identity(user, passwd, &cred);
+    if(!rc) {
+      rc = ldap_bind_s(server, NULL, (TCHAR *)&cred, method);
+      Curl_sspi_free_identity(&cred);
+    }
+  }
+  else {
+    /* proceed with current user credentials */
+    method = LDAP_AUTH_NEGOTIATE;
+    rc = ldap_bind_s(server, NULL, NULL, method);
+  }
+  return rc;
+}
+#endif /* #if defined(USE_WINDOWS_SSPI) */
+
+static int ldap_win_bind(struct connectdata *conn, LDAP *server,
+                         const char *user, const char *passwd)
+{
+  int rc = LDAP_INVALID_CREDENTIALS;
+  ULONG method = LDAP_AUTH_SIMPLE;
+
+  PTCHAR inuser = NULL;
+  PTCHAR inpass = NULL;
+
+  if(user && passwd && (conn->data->set.httpauth & CURLAUTH_BASIC)) {
+    inuser = Curl_convert_UTF8_to_tchar((char*)user);
+    inpass = Curl_convert_UTF8_to_tchar((char*)passwd);
+
+    rc = ldap_bind_s(server, inuser, inpass, method);
+
+    Curl_unicodefree(inuser);
+    Curl_unicodefree(inpass);
+  }
+#if defined(USE_WINDOWS_SSPI)
+  else {
+    rc = ldap_win_bind_auth(server, user, passwd, conn->data->set.httpauth);
+  }
+#endif
+
+  return rc;
+}
+#endif /* #if defined(USE_WIN32_LDAP) */
 
 static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
 {
@@ -202,13 +277,11 @@ static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
 #endif
 #if defined(USE_WIN32_LDAP)
   TCHAR *host = NULL;
-  TCHAR *user = NULL;
-  TCHAR *passwd = NULL;
 #else
   char *host = NULL;
+#endif
   char *user = NULL;
   char *passwd = NULL;
-#endif
 
   *done = TRUE; /* unconditionally */
   infof(data, "LDAP local: LDAP Vendor = %s ; LDAP Version = %d\n",
@@ -239,24 +312,14 @@ static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
 
     goto quit;
   }
-
-  if(conn->bits.user_passwd) {
-    user = Curl_convert_UTF8_to_tchar(conn->user);
-    passwd = Curl_convert_UTF8_to_tchar(conn->passwd);
-    if(!user || !passwd) {
-      result = CURLE_OUT_OF_MEMORY;
-
-      goto quit;
-    }
-  }
 #else
   host = conn->host.name;
+#endif
 
   if(conn->bits.user_passwd) {
     user = conn->user;
     passwd = conn->passwd;
   }
-#endif
 
 #ifdef LDAP_OPT_NETWORK_TIMEOUT
   ldap_set_option(NULL, LDAP_OPT_NETWORK_TIMEOUT, &ldap_timeout);
@@ -402,11 +465,19 @@ static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
   ldap_set_option(server, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);
 #endif
 
+#ifdef USE_WIN32_LDAP
+  rc = ldap_win_bind(conn, server, user, passwd);
+#else
   rc = ldap_simple_bind_s(server, user, passwd);
+#endif
   if(!ldap_ssl && rc != 0) {
     ldap_proto = LDAP_VERSION2;
     ldap_set_option(server, LDAP_OPT_PROTOCOL_VERSION, &ldap_proto);
+#ifdef USE_WIN32_LDAP
+    rc = ldap_win_bind(conn, server, user, passwd);
+#else
     rc = ldap_simple_bind_s(server, user, passwd);
+#endif
   }
   if(rc != 0) {
     failf(data, "LDAP local: ldap_simple_bind_s %s", ldap_err2string(rc));
@@ -669,8 +740,6 @@ static CURLcode Curl_ldap(struct connectdata *conn, bool *done)
 #endif /* HAVE_LDAP_SSL && CURL_HAS_NOVELL_LDAPSDK */
 
 #if defined(USE_WIN32_LDAP)
-  Curl_unicodefree(passwd);
-  Curl_unicodefree(user);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-15e9c21912` → 本草稿移入 `cases/defect/auto-curl-15e9c21912/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
