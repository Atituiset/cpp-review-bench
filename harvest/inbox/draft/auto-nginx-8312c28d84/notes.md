# auto-nginx-8312c28d84

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1648 (https://github.com/nginx/nginx/pull/1648)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 1078；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1075,7 +1075,7 @@ ngx_http_uwsgi_create_request(ngx_http_request_t *r)
         while (*(uintptr_t *) le.ip) {
 
             lcode = *(ngx_http_script_len_code_pt *) le.ip;
-            key_len = (u_char) lcode(&le);
+            key_len = lcode(&le);
 
             lcode = *(ngx_http_script_len_code_pt *) le.ip;
             skip_empty = lcode(&le);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-8312c28d84` → 本草稿移入 `cases/defect/auto-nginx-8312c28d84/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
