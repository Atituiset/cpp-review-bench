# auto-curl-74f9825dbf

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1549 (https://github.com/curl/curl/pull/1549)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -5,7 +5,7 @@
  *                            | (__| |_| |  _ <| |___
  *                             \___|\___/|_| \_\_____|
  *
- * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
+ * Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
  *
  * This software is licensed as described in the file COPYING, which
  * you should have received as part of this distribution. The terms
@@ -246,27 +246,29 @@ static CURLcode getinfo_long(struct Curl_easy *data, CURLINFO info,
   return CURLE_OK;
 }
 
+#define DOUBLE_SECS(x) (double)(x)/1000000
+
 static CURLcode getinfo_double(struct Curl_easy *data, CURLINFO info,
                                double *param_doublep)
 {
   switch(info) {
   case CURLINFO_TOTAL_TIME:
-    *param_doublep = data->progress.timespent;
+    *param_doublep = DOUBLE_SECS(data->progress.timespent);
     break;
   case CURLINFO_NAMELOOKUP_TIME:
-    *param_doublep = data->progress.t_nslookup;
+    *param_doublep = DOUBLE_SECS(data->progress.t_nslookup);
     break;
   case CURLINFO_CONNECT_TIME:
-    *param_doublep = data->progress.t_connect;
+    *param_doublep = DOUBLE_SECS(data->progress.t_connect);
     break;
   case CURLINFO_APPCONNECT_TIME:
-    *param_doublep = data->progress.t_appconnect;
+    *param_doublep = DOUBLE_SECS(data->progress.t_appconnect);
     break;
   case CURLINFO_PRETRANSFER_TIME:
-    *param_doublep =  data->progress.t_pretransfer;
+    *param_doublep = DOUBLE_SECS(data->progress.t_pretransfer);
     break;
   case CURLINFO_STARTTRANSFER_TIME:
-    *param_doublep = data->progress.t_starttransfer;
+    *param_doublep = DOUBLE_SECS(data->progress.t_starttransfer);
     break;
   case CURLINFO_SIZE_UPLOAD:
     *param_doublep =  (double)data->progress.uploaded;
@@ -289,7 +291,7 @@ static CURLcode getinfo_double(struct Curl_easy *data, CURLINFO info,
       (double)data->progress.size_ul:-1;
     break;
   case CURLINFO_REDIRECT_TIME:
-    *param_doublep =  data->progress.t_redirect;
+    *param_doublep = DOUBLE_SECS(data->progress.t_redirect);
     break;
 
   default:
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-74f9825dbf` → 本草稿移入 `cases/defect/auto-curl-74f9825dbf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
