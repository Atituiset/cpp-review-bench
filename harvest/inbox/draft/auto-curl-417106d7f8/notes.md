# auto-curl-417106d7f8

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #6745 (https://github.com/curl/curl/pull/6745)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 20（原始 PR diff 行 213；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -171,65 +171,63 @@ singleipconnect(struct Curl_easy *data,
  * infinite time left). If the value is negative, the timeout time has already
  * elapsed.
  *
- * The start time is stored in progress.t_startsingle - as set with
- * Curl_pgrsTime(..., TIMER_STARTSINGLE);
- *
  * If 'nowp' is non-NULL, it points to the current time.
  * 'duringconnect' is FALSE if not during a connect, as then of course the
  * connect timeout is not taken into account!
  *
  * @unittest: 1303
  */
+
+#define TIMEOUT_CONNECT 1
+#define TIMEOUT_MAXTIME 2
+
 timediff_t Curl_timeleft(struct Curl_easy *data,
                          struct curltime *nowp,
                          bool duringconnect)
 {
-  int timeout_set = 0;
-  timediff_t timeout_ms = duringconnect?DEFAULT_CONNECT_TIMEOUT:0;
+  unsigned int timeout_set = 0;
+  timediff_t connect_timeout_ms = 0;
+  timediff_t maxtime_timeout_ms = 0;
+  timediff_t timeout_ms = 0;
   struct curltime now;
 
-  /* if a timeout is set, use the most restrictive one */
-
-  if(data->set.timeout > 0)
-    timeout_set |= 1;
-  if(duringconnect && (data->set.connecttimeout > 0))
-    timeout_set |= 2;
-
-  switch(timeout_set) {
-  case 1:
-    timeout_ms = data->set.timeout;
-    break;
-  case 2:
-    timeout_ms = data->set.connecttimeout;
-    break;
-  case 3:
-    if(data->set.timeout < data->set.connecttimeout)
-      timeout_ms = data->set.timeout;
-    else
-      timeout_ms = data->set.connecttimeout;
-    break;
-  default:
-    /* use the default */
-    if(!duringconnect)
-      /* if we're not during connect, there's no default timeout so if we're
-         at zero we better just return zero and not make it a negative number
-         by the math below */
-      return 0;
-    break;
+  /* The duration of a connect and the total transfer are calculated from two
+     different time-stamps. It can end up with the total timeout being reached
+     before the connect timeout expires and we must acknowledge whichever
+     timeout that is reached first. The total timeout is set per entire
+     operation, while the connect timeout is set per connect. */
+
+  if(data->set.timeout > 0) {
+    timeout_set = TIMEOUT_MAXTIME;
+    maxtime_timeout_ms = data->set.timeout;
+  }
+  if(duringconnect) {
+    timeout_set |= TIMEOUT_CONNECT;
+    connect_timeout_ms = (data->set.connecttimeout > 0) ?
+      data->set.connecttimeout : DEFAULT_CONNECT_TIMEOUT;
   }
+  if(!timeout_set)
+    /* no timeout  */
+    return 0;
 
   if(!nowp) {
     now = Curl_now();
     nowp = &now;
   }
 
-  /* subtract elapsed time */
-  if(duringconnect)
-    /* since this most recent connect started */
-    timeout_ms -= Curl_timediff(*nowp, data->progress.t_startsingle);
-  else
-    /* since the entire operation started */
-    timeout_ms -= Curl_timediff(*nowp, data->progress.t_startop);
+  if(timeout_set & TIMEOUT_MAXTIME) {
+    maxtime_timeout_ms -= Curl_timediff(*nowp, data->progress.t_startop);
+    timeout_ms = maxtime_timeout_ms;
+  }
+
+  if(timeout_set & TIMEOUT_CONNECT) {
+    connect_timeout_ms -= Curl_timediff(*nowp, data->progress.t_startsingle);
+
+    if(!(timeout_set & TIMEOUT_MAXTIME) ||
+       (connect_timeout_ms < maxtime_timeout_ms))
+      timeout_ms = connect_timeout_ms;
+  }
+
   if(!timeout_ms)
     /* avoid returning 0 as that means no timeout! */
     return -1;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-417106d7f8` → 本草稿移入 `cases/defect/auto-curl-417106d7f8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
