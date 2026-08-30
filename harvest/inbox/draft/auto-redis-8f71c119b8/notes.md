# auto-redis-8f71c119b8

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15252 (https://github.com/redis/redis/pull/15252)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -168,13 +168,45 @@ typedef struct raxIterator {
     void *privdata;         /* Optional private data for node callback. */
 } raxIterator;
 
+/* Result of a rax walk, used to commit a find-then-insert pair without
+ * re-walking the tree.
+ *
+ *   raxFindLink()  produces a link.
+ *   raxInsertAt()  consumes it.
+ *
+ * Invalidation contract: `stopnode` and `parentlink` are interior pointers into
+ * the tree. They become stale on ANY intervening rax mutation. Callers
+ * MUST commit (or discard) immediately after the find; do not interleave
+ * other rax calls on the same tree, do not retain across yield points.
+ * The commit itself is allowed to realloc `stopnode` (raxReallocForData,
+ * raxAddChild) and update *parentlink in-place -- the link's own fields
+ * survive the commit, but a second commit on the same link is undefined. */
+typedef struct raxNodeLink {
+    raxNode  *stopnode;     /* Stop node. */
+    raxNode **parentlink;   /* Slot in stopnode's parent that holds h. */
+    size_t    consumed;     /* Bytes of key consumed at stop. */
+    int       splitpos;     /* Split position inside stopnode's compressed
+                             * prefix. Same semantic as raxLowWalk():
+                             * only meaningful when stopnode->iscompr; 0 with
+                             * i == len means clean arrival at stopnode, 0 with
+                             * i < len means the first prefix byte
+                             * mismatched the next key byte, > 0 means
+                             * the walk stopped mid-prefix. */
+} raxNodeLink;
+
 /* Exported API. */
 rax *raxNew(void);
 rax *raxNewWithMetadata(int metaSize, size_t *alloc_size);
 int raxInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);
 int raxTryInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);
 int raxRemove(rax *rax, unsigned char *s, size_t len, void **old);
 int raxFind(rax *rax, unsigned char *s, size_t len, void **value);
+
+int raxFindLink(rax *rax, unsigned char *s, size_t len,
+                void **value, raxNodeLink *link);
+int raxInsertAt(rax *rax, unsigned char *s, size_t len,
+                void *data, void **old, raxNodeLink *link);
+
 void raxFree(rax *rax);
 void raxFreeWithCallback(rax *rax, void (*free_callback)(void*));
 void raxFreeWithCbAndContext(rax *rax,
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8f71c119b8` → 本草稿移入 `cases/defect/auto-redis-8f71c119b8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
