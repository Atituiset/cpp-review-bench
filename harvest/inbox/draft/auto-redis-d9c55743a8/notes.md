# auto-redis-d9c55743a8

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14704 (https://github.com/redis/redis/pull/14704)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1240；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1235,9 +1235,23 @@ size_t kvobjComputeSize(robj *key, kvobj *o, size_t sample_size, int dbid) {
     serverPanic("Unknown object type");
 }
 
+/* Returns the size in bytes consumed by the object header, key and value in RAM.
+ * Note that the returned value is accurate approximation of the actual allocated
+ * size. For performance reasons it accumulates requested size instead in several
+ * cases (e.g. kvobj allocation, type 5 sds, listpacks, etc) but it does so in a
+ * self-consistent way.
+ */
 size_t kvobjAllocSize(kvobj *o) {
-    /* All kv-objects has at least kvobj header and embedded key */
-    size_t asize = zmalloc_size(kvobjGetAllocPtr(o));
+    debugServerAssert(o->iskvobj);
+    size_t asize = sizeof(kvobj);
+    /* Add metadata size */
+    asize += getNumMeta(o->metabits) * sizeof(uint64_t);
+    /* Add embedded key size */
+    asize += 1; /* embedded key header size */
+    asize += sdsAllocSize(kvobjGetKey(o));
+    /* Add embedded string size */
+    if (o->encoding == OBJ_ENCODING_EMBSTR)
+        asize += sdsAllocSize(o->ptr);
 
     if (o->type == OBJ_STRING) {
         asize += stringObjectAllocSize(o);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-d9c55743a8` → 本草稿移入 `cases/defect/auto-redis-d9c55743a8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
