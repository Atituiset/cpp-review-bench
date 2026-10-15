# auto-curl-c0a35c64cd

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1549 (https://github.com/curl/curl/pull/1549)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 155；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -152,10 +152,10 @@ int Curl_pgrsDone(struct connectdata *conn)
 /* reset all times except redirect, and reset the known transfer sizes */
 void Curl_pgrsResetTimesSizes(struct Curl_easy *data)
 {
-  data->progress.t_nslookup = 0.0;
-  data->progress.t_connect = 0.0;
-  data->progress.t_pretransfer = 0.0;
-  data->progress.t_starttransfer = 0.0;
+  data->progress.t_nslookup = 0;
+  data->progress.t_connect = 0;
+  data->progress.t_pretransfer = 0;
+  data->progress.t_starttransfer = 0;
 
   Curl_pgrsSetDownloadSize(data, -1);
   Curl_pgrsSetUploadSize(data, -1);
@@ -164,6 +164,7 @@ void Curl_pgrsResetTimesSizes(struct Curl_easy *data)
 void Curl_pgrsTime(struct Curl_easy *data, timerid timer)
 {
   struct timeval now = Curl_tvnow();
+  time_t *delta = NULL;
 
   switch(timer) {
   default:
@@ -178,38 +179,37 @@ void Curl_pgrsTime(struct Curl_easy *data, timerid timer)
     /* This is set at the start of each single fetch */
     data->progress.t_startsingle = now;
     break;
-
   case TIMER_STARTACCEPT:
-    data->progress.t_acceptdata = Curl_tvnow();
+    data->progress.t_acceptdata = now;
     break;
-
   case TIMER_NAMELOOKUP:
-    data->progress.t_nslookup =
-      Curl_tvdiff_secs(now, data->progress.t_startsingle);
+    delta = &data->progress.t_nslookup;
     break;
   case TIMER_CONNECT:
-    data->progress.t_connect =
-      Curl_tvdiff_secs(now, data->progress.t_startsingle);
+    delta = &data->progress.t_connect;
     break;
   case TIMER_APPCONNECT:
-    data->progress.t_appconnect =
-      Curl_tvdiff_secs(now, data->progress.t_startsingle);
+    delta = &data->progress.t_appconnect;
     break;
   case TIMER_PRETRANSFER:
-    data->progress.t_pretransfer =
-      Curl_tvdiff_secs(now, data->progress.t_startsingle);
+    delta = &data->progress.t_pretransfer;
     break;
   case TIMER_STARTTRANSFER:
-    data->progress.t_starttransfer =
-      Curl_tvdiff_secs(now, data->progress.t_startsingle);
+    delta = &data->progress.t_starttransfer;
     break;
   case TIMER_POSTRANSFER:
     /* this is the normal end-of-transfer thing */
     break;
   case TIMER_REDIRECT:
-    data->progress.t_redirect = Curl_tvdiff_secs(now, data->progress.start);
+    data->progress.t_redirect = Curl_tvdiff_us(now, data->progress.start);
     break;
   }
+  if(delta) {
+    time_t us = Curl_tvdiff_us(now, data->progress.t_startsingle);
+    if(!us)
+      us++; /* make sure at least one microsecond passed */
+    *delta = us;
+  }
 }
 
 void Curl_pgrsStartNow(struct Curl_easy *data)
@@ -361,17 +361,17 @@ int Curl_pgrsUpdate(struct connectdata *conn)
   now = Curl_tvnow(); /* what time is it */
 
   /* The time spent so far (from the start) */
-  data->progress.timespent = curlx_tvdiff_secs(now, data->progress.start);
+  data->progress.timespent = Curl_tvdiff_us(now, data->progress.start);
   timespent = (curl_off_t)data->progress.timespent;
 
   /* The average download speed this far */
   data->progress.dlspeed = (curl_off_t)
-    ((double)data->progress.downloaded/
+    (data->progress.downloaded/
      (data->progress.timespent>0?data->progress.timespent:1));
 
   /* The average upload speed this far */
   data->progress.ulspeed = (curl_off_t)
-    ((double)data->progress.uploaded/
+    (data->progress.uploaded/
      (data->progress.timespent>0?data->progress.timespent:1));
 
   /* Calculations done at most once a second, unless end is reached */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-c0a35c64cd` → 本草稿移入 `cases/defect/auto-curl-c0a35c64cd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
