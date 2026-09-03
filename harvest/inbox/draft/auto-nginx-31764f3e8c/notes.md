# auto-nginx-31764f3e8c

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #777 (https://github.com/nginx/nginx/pull/777)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 194；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -10,6 +10,15 @@
 #include <ngx_event.h>
 
 
+/* NetBSD up to 10.0 incompatibly defines kevent.udata as "intptr_t" */
+
+#if (__NetBSD__ && __NetBSD_Version__ < 1000000000)
+#define NGX_KQUEUE_UDATA_T
+#else
+#define NGX_KQUEUE_UDATA_T  (void *)
+#endif
+
+
 typedef struct {
     ngx_uint_t  changes;
     ngx_uint_t  events;
@@ -191,7 +200,7 @@ ngx_kqueue_init(ngx_cycle_t *cycle, ngx_msec_t timer)
         kev.flags = EV_ADD|EV_ENABLE;
         kev.fflags = 0;
         kev.data = timer;
-        kev.udata = 0;
+        kev.udata = NGX_KQUEUE_UDATA_T (uintptr_t) 0;
 
         ts.tv_sec = 0;
         ts.tv_nsec = 0;
@@ -237,7 +246,7 @@ ngx_kqueue_notify_init(ngx_log_t *log)
     notify_kev.data = 0;
     notify_kev.flags = EV_ADD|EV_CLEAR;
     notify_kev.fflags = 0;
-    notify_kev.udata = 0;
+    notify_kev.udata = NGX_KQUEUE_UDATA_T (uintptr_t) 0;
 
     if (kevent(ngx_kqueue, &notify_kev, 1, NULL, 0, NULL) == -1) {
         ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-31764f3e8c` → 本草稿移入 `cases/defect/auto-nginx-31764f3e8c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
