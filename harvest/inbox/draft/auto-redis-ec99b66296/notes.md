# auto-redis-ec99b66296

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15508 (https://github.com/redis/redis/pull/15508)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2286；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2276,17 +2276,25 @@ int raxRandomWalk(raxIterator *it, size_t steps) {
 
     raxNode *n = it->node;
     while(steps > 0 || (!n->iskey && it->leaf_slot_idx < 0)) {
+        /* A fixed-length inlined value is a virtual leaf: logically it has no
+         * children, even though it->node points to the leaf parent whose slots
+         * contain values. Move back to that parent before selecting the next
+         * edge. Otherwise the value slots could be interpreted as raxNode
+         * pointers when the walk continues from an already-positioned leaf. */
+        if (it->leaf_slot_idx >= 0) {
+            it->leaf_slot_idx = -1;
+            int todel = n->iscompr ? n->size : 1;
+            raxIteratorDelChars(it,todel);
+            continue;
+        }
+
         int numchildren = n->iscompr ? 1 : n->size;
         int r = rand() % (numchildren+(n != it->rt->head));
 
         if (r == numchildren) {
-            /* Go up: if parked on a virtual leaf, exit it (the leaf parent
-             * stays as n). Otherwise pop the real parent. Either way, the
-             * edge to strip is owned by the resulting `n`. */
-            if (it->leaf_slot_idx >= 0)
-                it->leaf_slot_idx = -1;
-            else
-                n = raxStackPop(&it->stack);
+            /* Go up to the real parent. The virtual-leaf case was normalized
+             * at the beginning of the loop. */
+            n = raxStackPop(&it->stack);
             int todel = n->iscompr ? n->size : 1;
             raxIteratorDelChars(it,todel);
         } else {
@@ -2868,6 +2876,31 @@ int raxTest(int argc, char **argv, int flags) {
         raxFree(r);
     }
 
+    TEST("inline-leaf: random walk from a virtual leaf") {
+        /* A single fixed-length key is stored as an iscompr=1 leaf parent at
+         * the root. Seeking positions the iterator on its virtual leaf. A
+         * subsequent random walk must first move back to the leaf parent;
+         * treating its value slot as a child raxNode would crash. Multiple
+         * steps exercise leaving and re-entering the same virtual leaf. */
+        rax *r = raxNewEx(0, NULL, 8);
+        unsigned char key[8] = {'A','A','A','A','A','A','A','A'};
+        void *value = (void*)(uintptr_t)1;
+        assert(raxInsert(r, key, sizeof(key), value, NULL) == 1);
+
+        raxIterator ri;
+        raxStart(&ri, r);
+        assert(raxSeek(&ri, "^", NULL, 0) == 1);
+        assert(ri.leaf_slot_idx == 0);
+        assert(raxRandomWalk(&ri, 8) == 1);
+        assert(ri.key_len == sizeof(key));
+        assert(memcmp(ri.key, key, sizeof(key)) == 0);
+        assert(ri.data == value);
+        assert(ri.leaf_slot_idx == 0);
+        raxStop(&ri);
+
+        raxFree(r);
+    }
+
     TEST("inline-leaf: delete triggers recompression into a sibling leaf parent") {
         /* k1 and k2 diverge at byte 1, each with a long unique suffix:
          *   "A" -> {A,B branch} -> "AAAAAA"(leaf parent, v1)   = k1
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-ec99b66296` → 本草稿移入 `cases/defect/auto-redis-ec99b66296/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
