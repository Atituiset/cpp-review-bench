# auto-nginx-d7686c5e47

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1212 (https://github.com/nginx/nginx/pull/1212)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1331；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1328,7 +1328,7 @@ ngx_mail_auth_http_create_request(ngx_mail_session_t *s, ngx_pool_t *pool,
         b->last = ngx_cpymem(b->last, "Auth-Salt: ", sizeof("Auth-Salt: ") - 1);
         b->last = ngx_copy(b->last, s->salt.data, s->salt.len);
 
-        s->passwd.data = NULL;
+        ngx_str_null(&s->passwd);
     }
 
     b->last = ngx_cpymem(b->last, "Auth-Protocol: ",
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-d7686c5e47` → 本草稿移入 `cases/defect/auto-nginx-d7686c5e47/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
