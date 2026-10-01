# auto-redis-ac89ead86a

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14927 (https://github.com/redis/redis/pull/14927)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1510；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1507,27 +1507,42 @@ int sdsTest(int argc, char **argv, int flags) {
         x = sdsResize(x, 200, 1);
         test_cond("sdsresize() expand len", sdslen(x) == 40);
         test_cond("sdsresize() expand strlen", strlen(x) == 40);
-        test_cond("sdsresize() expand alloc", sdsalloc(x) == 200);
+#if defined(USE_JEMALLOC)
+        /* 224 - hdrlen(3) - 1(\0) */
+        test_cond("sdsresize() expand alloc", sdsalloc(x) == 220);
+#endif
         /* Test sdsresize - trim free space */
         x = sdsResize(x, 80, 1);
         test_cond("sdsresize() shrink len", sdslen(x) == 40);
         test_cond("sdsresize() shrink strlen", strlen(x) == 40);
-        test_cond("sdsresize() shrink alloc", sdsalloc(x) == 80);
+#if defined(USE_JEMALLOC)
+        /* 96 - hdrlen(3) - 1(\0) */
+        test_cond("sdsresize() shrink alloc", sdsalloc(x) == 92);
+#endif
         /* Test sdsresize - crop used space */
         x = sdsResize(x, 30, 1);
         test_cond("sdsresize() crop len", sdslen(x) == 30);
         test_cond("sdsresize() crop strlen", strlen(x) == 30);
-        test_cond("sdsresize() crop alloc", sdsalloc(x) == 30);
+#if defined(USE_JEMALLOC)
+        /* 40 - hdrlen(3) - 1(\0) */
+        test_cond("sdsresize() crop alloc", sdsalloc(x) == 36);
+#endif
         /* Test sdsresize - extend to different class */
         x = sdsResize(x, 400, 1);
         test_cond("sdsresize() expand len", sdslen(x) == 30);
         test_cond("sdsresize() expand strlen", strlen(x) == 30);
-        test_cond("sdsresize() expand alloc", sdsalloc(x) == 400);
+#if defined(USE_JEMALLOC)
+        /* 448 - hdrlen(5) - 1(\0) */
+        test_cond("sdsresize() expand alloc", sdsalloc(x) == 442);
+#endif
         /* Test sdsresize - shrink to different class */
         x = sdsResize(x, 4, 1);
         test_cond("sdsresize() crop len", sdslen(x) == 4);
         test_cond("sdsresize() crop strlen", strlen(x) == 4);
+#if defined(USE_JEMALLOC)
+        /* 8 - hdrlen(3) - 1(\0) */
         test_cond("sdsresize() crop alloc", sdsalloc(x) == 4);
+#endif
         sdsfree(x);
         
         { /* Test adjustTypeIfNeeded() */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-ac89ead86a` → 本草稿移入 `cases/defect/auto-redis-ac89ead86a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
