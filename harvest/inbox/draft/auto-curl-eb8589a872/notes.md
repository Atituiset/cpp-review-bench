# auto-curl-eb8589a872

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #6193 (https://github.com/curl/curl/pull/6193)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 360；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -167,7 +167,7 @@ static CURLcode connect_init(struct connectdata *conn, bool reinit)
     Curl_dyn_reset(&s->rcvbuf);
   }
   s->tunnel_state = TUNNEL_INIT;
-  s->keepon = TRUE;
+  s->keepon = KEEPON_CONNECT;
   s->cl = 0;
   s->close_connection = FALSE;
   return CURLE_OK;
@@ -339,7 +339,7 @@ static CURLcode CONNECT(struct connectdata *conn,
           return CURLE_ABORTED_BY_CALLBACK;
 
         if(result) {
-          s->keepon = FALSE;
+          s->keepon = KEEPON_DONE;
           break;
         }
         else if(gotbytes <= 0) {
@@ -353,19 +353,19 @@ static CURLcode CONNECT(struct connectdata *conn,
             error = SELECT_ERROR;
             failf(data, "Proxy CONNECT aborted");
           }
-          s->keepon = FALSE;
+          s->keepon = KEEPON_DONE;
           break;
         }
 
-        if(s->keepon > TRUE) {
+        if(s->keepon == KEEPON_IGNORE) {
           /* This means we are currently ignoring a response-body */
 
           if(s->cl) {
             /* A Content-Length based body: simply count down the counter
                and make sure to break out of the loop when we're done! */
             s->cl--;
             if(s->cl <= 0) {
-              s->keepon = FALSE;
+              s->keepon = KEEPON_DONE;
               s->tunnel_state = TUNNEL_COMPLETE;
               break;
             }
@@ -383,7 +383,7 @@ static CURLcode CONNECT(struct connectdata *conn,
             if(r == CHUNKE_STOP) {
               /* we're done reading chunks! */
               infof(data, "chunk reading DONE\n");
-              s->keepon = FALSE;
+              s->keepon = KEEPON_DONE;
               /* we did the full CONNECT treatment, go COMPLETE */
               s->tunnel_state = TUNNEL_COMPLETE;
             }
@@ -437,7 +437,7 @@ static CURLcode CONNECT(struct connectdata *conn,
             /* If we get a 407 response code with content length
                when we have no auth problem, we must ignore the
                whole response-body */
-            s->keepon = 2;
+            s->keepon = KEEPON_IGNORE;
 
             if(s->cl) {
               infof(data, "Ignore %" CURL_FORMAT_CURL_OFF_T
@@ -465,7 +465,7 @@ static CURLcode CONNECT(struct connectdata *conn,
               if(r == CHUNKE_STOP) {
                 /* we're done reading chunks! */
                 infof(data, "chunk reading DONE\n");
-                s->keepon = FALSE;
+                s->keepon = KEEPON_DONE;
                 /* we did the full CONNECT treatment, go to COMPLETE */
                 s->tunnel_state = TUNNEL_COMPLETE;
               }
@@ -474,11 +474,11 @@ static CURLcode CONNECT(struct connectdata *conn,
               /* without content-length or chunked encoding, we
                  can't keep the connection alive since the close is
                  the end signal so we bail out at once instead */
-              s->keepon = FALSE;
+              s->keepon = KEEPON_DONE;
             }
           }
           else
-            s->keepon = FALSE;
+            s->keepon = KEEPON_DONE;
           if(!s->cl)
             /* we did the full CONNECT treatment, go to COMPLETE */
             s->tunnel_state = TUNNEL_COMPLETE;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-eb8589a872` → 本草稿移入 `cases/defect/auto-curl-eb8589a872/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
