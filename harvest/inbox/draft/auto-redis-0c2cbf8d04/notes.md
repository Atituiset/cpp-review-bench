# auto-redis-0c2cbf8d04

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15436 (https://github.com/redis/redis/pull/15436)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2495,6 +2495,17 @@ int rewriteModuleObject(rio *r, robj *key, robj *o, int dbid) {
     RedisModuleIO io;
     moduleValue *mv = o->ptr;
     moduleType *mt = mv->type;
+    /* The aof_rewrite callback is optional for a module data type. Calling it
+     * when it is NULL would crash the child process performing the AOF
+     * rewrite, so fail the rewrite with a clear error instead. */
+    if (mt->aof_rewrite == NULL) {
+        serverLog(LL_WARNING,
+            "Can't rewrite the append only file: the module data type '%s' "
+            "does not implement the aof_rewrite callback. Enable "
+            "aof-use-rdb-preamble to rewrite the AOF for this data type.",
+            mt->entity.name);
+        return 0;
+    }
     moduleInitIOContext(&io, &mt->entity, r, key, dbid);
     mt->aof_rewrite(&io,key,mv->value);
     if (io.ctx) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0c2cbf8d04` → 本草稿移入 `cases/defect/auto-redis-0c2cbf8d04/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
