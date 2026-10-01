# auto-nginx-49b10894ed

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #931 (https://github.com/nginx/nginx/pull/931)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 538；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -535,9 +535,15 @@ ngx_init_cycle(ngx_cycle_t *old_cycle)
                     == NGX_OK)
                 {
                     nls[n].fd = ls[i].fd;
-                    nls[n].inherited = ls[i].inherited;
                     nls[n].previous = &ls[i];
-                    ls[i].remain = 1;
+
+                    if (ls[i].protocol != nls[n].protocol) {
+                        nls[n].change_protocol = 1;
+
+                    } else {
+                        nls[n].inherited = ls[i].inherited;
+                        ls[i].remain = 1;
+                    }
 
                     if (ls[i].backlog != nls[n].backlog) {
                         nls[n].listen = 1;
@@ -590,7 +596,6 @@ ngx_init_cycle(ngx_cycle_t *old_cycle)
             }
 
             if (nls[n].fd == (ngx_socket_t) -1) {
-                nls[n].open = 1;
 #if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
                 if (nls[n].accept_filter) {
                     nls[n].add_deferred = 1;
@@ -605,20 +610,21 @@ ngx_init_cycle(ngx_cycle_t *old_cycle)
         }
 
     } else {
+#if (NGX_HAVE_DEFERRED_ACCEPT)
         ls = cycle->listening.elts;
         for (i = 0; i < cycle->listening.nelts; i++) {
-            ls[i].open = 1;
-#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
+#ifdef SO_ACCEPTFILTER
             if (ls[i].accept_filter) {
                 ls[i].add_deferred = 1;
             }
 #endif
-#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
+#ifdef TCP_DEFER_ACCEPT
             if (ls[i].deferred_accept) {
                 ls[i].add_deferred = 1;
             }
 #endif
         }
+#endif
     }
 
     if (ngx_open_listening_sockets(cycle) != NGX_OK) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-49b10894ed` → 本草稿移入 `cases/defect/auto-nginx-49b10894ed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
