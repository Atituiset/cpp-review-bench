# auto-nginx-6e6ac571bd

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #124 (https://github.com/nginx/nginx/pull/124)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -623,6 +623,12 @@ ngx_http_v3_parse_literal(ngx_connection_t *c, ngx_http_v3_parse_literal_t *st,
             }
 
             if (st->huffman) {
+                if (n > NGX_MAX_INT_T_VALUE / 8) {
+                    ngx_log_error(NGX_LOG_INFO, c->log, 0,
+                                  "client sent too large field line");
+                    return NGX_HTTP_V3_ERR_EXCESSIVE_LOAD;
+                }
+
                 n = n * 8 / 5;
                 st->huffstate = 0;
             }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-6e6ac571bd` → 本草稿移入 `cases/defect/auto-nginx-6e6ac571bd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
