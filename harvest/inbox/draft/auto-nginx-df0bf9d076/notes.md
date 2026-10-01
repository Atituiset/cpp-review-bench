# auto-nginx-df0bf9d076

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #833 (https://github.com/nginx/nginx/pull/833)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 256；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -253,7 +253,8 @@ ngx_http_auth_basic_handler(ngx_http_request_t *r)
         pwd.len = i - passwd;
         pwd.data = ngx_pnalloc(r->pool, pwd.len + 1);
         if (pwd.data == NULL) {
-            return NGX_HTTP_INTERNAL_SERVER_ERROR;
+            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
+            goto cleanup;
         }
 
         ngx_cpystrn(pwd.data, &buf[passwd], pwd.len + 1);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-df0bf9d076` → 本草稿移入 `cases/defect/auto-nginx-df0bf9d076/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
