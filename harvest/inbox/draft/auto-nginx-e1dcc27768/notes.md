# auto-nginx-e1dcc27768

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1167 (https://github.com/nginx/nginx/pull/1167)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 71；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -60,6 +60,11 @@ struct ngx_peer_connection_s {
 
     ngx_log_t                       *log;
 
+#if (NGX_HTTP_UPSTREAM_SID || NGX_COMPAT)
+    ngx_str_t                       *hint;
+    ngx_str_t                       *sid;
+#endif
+
     unsigned                         cached:1;
     unsigned                         transparent:1;
     unsigned                         so_keepalive:1;
@@ -68,7 +73,7 @@ struct ngx_peer_connection_s {
                                      /* ngx_connection_log_error_e */
     unsigned                         log_error:2;
 
-    NGX_COMPAT_BEGIN(2)
+    NGX_COMPAT_BEGIN(1)
     NGX_COMPAT_END
 };
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-e1dcc27768` → 本草稿移入 `cases/defect/auto-nginx-e1dcc27768/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
