# auto-curl-d22ae7784c

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7278 (https://github.com/curl/curl/pull/7278)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 341；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -330,6 +330,9 @@ set_ssl_version_min_max(struct Curl_easy *data,
       ssl_version_max = CURL_SSLVERSION_MAX_TLSv1_2;
     }
   }
+  else if(ssl_version_max == CURL_SSLVERSION_MAX_DEFAULT) {
+    ssl_version_max = CURL_SSLVERSION_MAX_TLSv1_3;
+  }
 
   switch(ssl_version | ssl_version_max) {
   case CURL_SSLVERSION_TLSv1_0 | CURL_SSLVERSION_MAX_TLSv1_0:
@@ -338,19 +341,19 @@ set_ssl_version_min_max(struct Curl_easy *data,
     return CURLE_OK;
   case CURL_SSLVERSION_TLSv1_0 | CURL_SSLVERSION_MAX_TLSv1_1:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.0:+VERS-TLS1.1";
+      "+VERS-TLS1.1:+VERS-TLS1.0";
     return CURLE_OK;
   case CURL_SSLVERSION_TLSv1_0 | CURL_SSLVERSION_MAX_TLSv1_2:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.0:+VERS-TLS1.1:+VERS-TLS1.2";
+      "+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0";
     return CURLE_OK;
   case CURL_SSLVERSION_TLSv1_1 | CURL_SSLVERSION_MAX_TLSv1_1:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
       "+VERS-TLS1.1";
     return CURLE_OK;
   case CURL_SSLVERSION_TLSv1_1 | CURL_SSLVERSION_MAX_TLSv1_2:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.1:+VERS-TLS1.2";
+      "+VERS-TLS1.2:+VERS-TLS1.1";
     return CURLE_OK;
   case CURL_SSLVERSION_TLSv1_2 | CURL_SSLVERSION_MAX_TLSv1_2:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
@@ -360,25 +363,16 @@ set_ssl_version_min_max(struct Curl_easy *data,
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
       "+VERS-TLS1.3";
     return CURLE_OK;
-  case CURL_SSLVERSION_TLSv1_0 | CURL_SSLVERSION_MAX_DEFAULT:
-    *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.0:+VERS-TLS1.1:+VERS-TLS1.2"
-      ":+VERS-TLS1.3";
-    return CURLE_OK;
-  case CURL_SSLVERSION_TLSv1_1 | CURL_SSLVERSION_MAX_DEFAULT:
-    *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.1:+VERS-TLS1.2"
-      ":+VERS-TLS1.3";
+  case CURL_SSLVERSION_TLSv1_0 | CURL_SSLVERSION_MAX_TLSv1_3:
+    *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0";
     return CURLE_OK;
-  case CURL_SSLVERSION_TLSv1_2 | CURL_SSLVERSION_MAX_DEFAULT:
+  case CURL_SSLVERSION_TLSv1_1 | CURL_SSLVERSION_MAX_TLSv1_3:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.2"
-      ":+VERS-TLS1.3";
+      "+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.1";
     return CURLE_OK;
-  case CURL_SSLVERSION_TLSv1_3 | CURL_SSLVERSION_MAX_DEFAULT:
+  case CURL_SSLVERSION_TLSv1_2 | CURL_SSLVERSION_MAX_TLSv1_3:
     *prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0:-VERS-TLS-ALL:"
-      "+VERS-TLS1.2"
-      ":+VERS-TLS1.3";
+      "+VERS-TLS1.3:+VERS-TLS1.2";
     return CURLE_OK;
   }
 
@@ -608,6 +602,7 @@ gtls_connect_step1(struct Curl_easy *data,
   }
   else {
 #endif
+    infof(data, "GnuTLS ciphers: %s\n", prioritylist);
     rc = gnutls_priority_set_direct(session, prioritylist, &err);
 #ifdef HAVE_GNUTLS_SRP
   }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-d22ae7784c` → 本草稿移入 `cases/defect/auto-curl-d22ae7784c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
