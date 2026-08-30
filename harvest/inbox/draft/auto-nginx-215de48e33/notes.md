# auto-nginx-215de48e33

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #344 (https://github.com/nginx/nginx/pull/344)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1995,6 +1995,11 @@ ngx_http_scgi_store(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
         return NGX_CONF_OK;
     }
 
+    if (value[1].len == 0) {
+        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "empty path");
+        return NGX_CONF_ERROR;
+    }
+
 #if (NGX_HTTP_CACHE)
     if (scf->upstream.cache > 0) {
         return "is incompatible with \"scgi_cache\"";
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-215de48e33` → 本草稿移入 `cases/defect/auto-nginx-215de48e33/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
