# auto-redis-2beba08dee

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15539 (https://github.com/redis/redis/pull/15539)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1312；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1309,7 +1309,7 @@ ssize_t rdbSaveObject(rio *rdb, robj *o, robj *key, int dbid) {
                 if (fields_lp) {
                     /* Get listpack blob and skip caching in fork. */
                     int cache = (server.in_fork_child == CHILD_TYPE_NONE);
-                    unsigned char *blob = hashTemplateGetFieldsLp(tmpl, cache);
+                    unsigned char *blob = hashTemplateGetFieldsLp(tmpl, &cache);
                     n = rdbSaveRawString(rdb, blob, lpBytes(blob));
                     if (!cache) lpFree(blob);
                     if (n == -1) return -1;
@@ -2870,7 +2870,8 @@ static hashTemplate *rdbCreateTemplateFromFields(rdbTmplFields *out) {
     rdbFreeSdsArray(out->fields, out->field_count);
     out->fields = NULL;
     if (out->fields_lp != NULL) {
-        hashTemplateIndexFieldsLp(tmpl, out->fields_lp); /* transfers ownership */
+        if (!hashTemplateIndexFieldsLp(tmpl, out->fields_lp))
+            lpFree(out->fields_lp);
         out->fields_lp = NULL;
     }
     return tmpl;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-2beba08dee` → 本草稿移入 `cases/defect/auto-redis-2beba08dee/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
