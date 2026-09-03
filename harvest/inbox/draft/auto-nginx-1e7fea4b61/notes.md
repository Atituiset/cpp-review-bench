# auto-nginx-1e7fea4b61

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #702 (https://github.com/nginx/nginx/pull/702)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 697；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -694,7 +694,7 @@ ngx_ssl_cache_pkey_create(ngx_ssl_cache_key_t *id, char **err, void *data)
             return NULL;
         }
 
-        pkey = ENGINE_load_private_key(engine, (char *) last, 0, 0);
+        pkey = ENGINE_load_private_key(engine, (char *) last, NULL, NULL);
 
         if (pkey == NULL) {
             *err = "ENGINE_load_private_key() failed";
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-1e7fea4b61` → 本草稿移入 `cases/defect/auto-nginx-1e7fea4b61/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
