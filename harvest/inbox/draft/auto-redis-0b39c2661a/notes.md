# auto-redis-0b39c2661a

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15225 (https://github.com/redis/redis/pull/15225)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -126,7 +126,7 @@ hash_table_add(hashTable *tbl, void *ptr, size_t bytes,
 }
 
 static void
-#if defined(__GNUC__) && __GNUC__ >= 10
+#if defined(__GNUC__) && __GNUC__ >= 11
 __attribute__((access(none, 2)))
 #endif
 hash_table_del(hashTable *tbl, void *ptr)
@@ -344,12 +344,12 @@ xrealloc_impl(void *ptr, size_t new_size, const char *file, int line,
   new_ptr = realloc(ptr, new_size);
   if (new_ptr != NULL && new_ptr != ptr)
     {
-#if defined(__GNUC__) && !defined(__clang__)
+#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wuse-after-free"
 #endif
       hash_table_del(xmalloc_table, ptr);
-#if defined(__GNUC__) && !defined(__clang__)
+#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12
 #pragma GCC diagnostic pop
 #endif
       hash_table_add(xmalloc_table, new_ptr, (int)new_size, file, line, func);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0b39c2661a` → 本草稿移入 `cases/defect/auto-redis-0b39c2661a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
