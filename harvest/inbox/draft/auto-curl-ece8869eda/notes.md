# auto-curl-ece8869eda

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1017 (https://github.com/curl/curl/pull/1017)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 61；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -36,8 +36,8 @@
 #include "memdebug.h"
 
 /*
- * This is supposed to be called in the beginning of a perform() session
- * and should reset all session-info variables
+ * This is supposed to be called in the beginning of a perform() session and
+ * in curl_easy_reset() and should reset all session-info variables.
  */
 CURLcode Curl_initinfo(struct Curl_easy *data)
 {
@@ -58,18 +58,27 @@ CURLcode Curl_initinfo(struct Curl_easy *data)
   info->filetime = -1; /* -1 is an illegal time and thus means unknown */
   info->timecond = FALSE;
 
-  free(info->contenttype);
-  info->contenttype = NULL;
-
   info->header_size = 0;
   info->request_size = 0;
+  info->proxyauthavail = 0;
+  info->httpauthavail = 0;
   info->numconnects = 0;
 
+  free(info->contenttype);
+  info->contenttype = NULL;
+
+  free(info->wouldredirect);
+  info->wouldredirect = NULL;
+
   info->conn_primary_ip[0] = '\0';
   info->conn_local_ip[0] = '\0';
   info->conn_primary_port = 0;
   info->conn_local_port = 0;
 
+#ifdef USE_SSL
+  Curl_ssl_free_certinfo(data);
+#endif
+
   return CURLE_OK;
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-ece8869eda` → 本草稿移入 `cases/defect/auto-curl-ece8869eda/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
