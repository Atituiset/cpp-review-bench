# auto-nginx-2b302b6558

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #143 (https://github.com/nginx/nginx/pull/143)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 453；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -450,9 +450,13 @@ ngx_mail_ssl_merge_conf(ngx_conf_t *cf, void *parent, void *child)
 
     if (conf->verify) {
 
-        if (conf->client_certificate.len == 0 && conf->verify != 3) {
+        if (conf->verify != 3
+            && conf->client_certificate.len == 0
+            && conf->trusted_certificate.len == 0)
+        {
             ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
-                          "no ssl_client_certificate for ssl_verify_client");
+                          "no ssl_client_certificate or "
+                          "ssl_trusted_certificate for ssl_verify_client");
             return NGX_CONF_ERROR;
         }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-2b302b6558` → 本草稿移入 `cases/defect/auto-nginx-2b302b6558/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
