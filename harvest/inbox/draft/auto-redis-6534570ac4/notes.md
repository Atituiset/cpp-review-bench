# auto-redis-6534570ac4

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15252 (https://github.com/redis/redis/pull/15252)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 958；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -524,25 +524,54 @@ static inline size_t raxLowWalk(rax *rax, unsigned char *s, size_t len, raxNode
     return i;
 }
 
-/* Insert the element 's' of size 'len', setting as auxiliary data
- * the pointer 'data'. If the element is already present, the associated
- * data is updated (only if 'overwrite' is set to 1), and 0 is returned,
- * otherwise the element is inserted and 1 is returned. On out of memory the
- * function returns 0 as well but sets errno to ENOMEM, otherwise errno will
- * be set to 0.
- */
-int raxGenericInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old, int overwrite) {
-    size_t i, usable;
-    int j = 0; /* Split position. If raxLowWalk() stops in a compressed
-                  node, the index 'j' represents the char we stopped within the
-                  compressed node, that is, the position where to split the
-                  node for insertion. */
-    raxNode *h, **parentlink;
+#ifdef DEBUG_ASSERTIONS
+/* Re-walk the tree and verify `link` still matches the current state,
+ * i.e. no rax mutation happened between raxFindLink() and the current
+ * raxInsertAt(). Returns 1 if the link is still valid, 0 otherwise.
+ * Used only from a debugAssert(). */
+static int raxLinkStillValid(rax *rax, unsigned char *s, size_t len, raxNodeLink *link) {
+    raxNode *stopnode, **parentlink;
+    int splitpos = 0;
+    size_t consumed = raxLowWalk(rax,s,len,&stopnode,&parentlink,&splitpos,NULL);
+    return stopnode == link->stopnode &&
+           parentlink == link->parentlink &&
+           consumed == link->consumed &&
+           splitpos == link->splitpos;
+}
+#endif
+
+/* Commit an insert at the position recorded in `link`. The link must
+ * have come from an immediately-preceding raxFindLink() on (rax, s, len)
+ * with no intervening rax mutation.
+ *
+ * If the link lands on an existing key, the associated data is
+ * overwritten with `data`, the prior value is stored at *old (when old
+ * is non-NULL), and 0 is returned. Otherwise the element is inserted
+ * and 1 is returned. Callers wanting try-insert semantics (preserve
+ * existing) should check raxFindLink's return first and skip this call
+ * when it reports 1.
+ *
+ * On out of memory the function returns 0 and sets errno to ENOMEM;
+ * otherwise errno is set to 0. */
+int raxInsertAt(rax *rax, unsigned char *s, size_t len, void *data, void **old, raxNodeLink *link) {
+    size_t usable;
+    /* Pull walk state from `link`. */
+    size_t i = link->consumed;
+    int j = link->splitpos; /* Split position. If raxLowWalk() stopped in
+                               a compressed node, 'j' is the char index
+                               within the compressed node where we
+                               stopped; i.e. the position where to split
+                               the node for insertion. Only meaningful
+                               when h->iscompr. */
+    raxNode *h = link->stopnode, **parentlink = link->parentlink;
     size_t dummy, *alloc_size = &dummy;
 
+    /* The link must reflect the current tree: no rax mutation is allowed
+     * between the raxFindLink() that produced it and this commit. */
+    debugAssert(raxLinkStillValid(rax,s,len,link));
+
     if (rax->alloc_size) alloc_size = rax->alloc_size;
     debugf("### Insert %.*s with value %p\n", (int)len, s, data);
-    i = raxLowWalk(rax,s,len,&h,&parentlink,&j,NULL);
 
     /* If i == len we walked following the whole string. If we are not
      * in the middle of a compressed node, the string is either already
@@ -552,7 +581,7 @@ int raxGenericInsert(rax *rax, unsigned char *s, size_t len, void *data, void **
     if (i == len && (!h->iscompr || j == 0 /* not in the middle if j is 0 */)) {
         debugf("### Insert: node representing key exists\n");
         /* Make space for the value pointer if needed. */
-        if (!h->iskey || (h->isnull && overwrite)) {
+        if (!h->iskey || h->isnull) {
             h = raxReallocForData(rax,h,da
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-6534570ac4` → 本草稿移入 `cases/defect/auto-redis-6534570ac4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
