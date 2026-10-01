# auto-curl-937fffbabd

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1549 (https://github.com/curl/curl/pull/1549)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 137；PR 修复前的代码，待确认是否为 bug）

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
@@ -130,14 +130,24 @@ time_t curlx_tvdiff(struct timeval newer, struct timeval older)
 }
 
 /*
- * Same as curlx_tvdiff but with full usec resolution.
+ * Make sure that the first argument is the more recent time, as otherwise
+ * we'll get a weird negative time-diff back...
  *
- * Returns: the time difference in seconds with subsecond resolution.
+ * Returns: the time difference in number of microseconds. For too large diffs
+ * it returns max value.
  */
-double curlx_tvdiff_secs(struct timeval newer, struct timeval older)
+time_t Curl_tvdiff_us(struct timeval newer, struct timeval older)
 {
-  if(newer.tv_sec != older.tv_sec)
-    return (double)(newer.tv_sec-older.tv_sec)+
-      (double)(newer.tv_usec-older.tv_usec)/1000000.0;
-  return (double)(newer.tv_usec-older.tv_usec)/1000000.0;
+  time_t diff = newer.tv_sec-older.tv_sec;
+#if SIZEOF_TIME_T < 8
+  /* for 32bit time_t systems */
+  if(diff >= (0x7fffffff/1000000))
+    return 0x7fffffff;
+#else
+  /* for 64bit time_t systems */
+  if(diff >= (0x7fffffffffffffff/1000000))
+    return 0x7fffffffffffffff;
+#endif
+  return (newer.tv_sec-older.tv_sec)*1000000+
+    (time_t)(newer.tv_usec-older.tv_usec);
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-937fffbabd` → 本草稿移入 `cases/defect/auto-curl-937fffbabd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
