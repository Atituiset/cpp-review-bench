# auto-redis-7a48ad2108

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15096 (https://github.com/redis/redis/pull/15096)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -183,4 +183,23 @@
 #error "Unable to determine atomic operations for your platform"
 
 #endif
+
+/* atomicIncrGetSingleWriter(var, delta, newvalue_var)
+ *
+ * Adds `delta` to `var` and writes the resulting value to `newvalue_var`.
+ * Same end result as atomicIncrGet() but implemented as load+add+store instead
+ * of an atomic read-modify-write. This avoids the `lock` prefix on x86
+ * (~20-40 cycles vs ~2-3 for plain load+store).
+ *
+ * SAFETY: the caller MUST guarantee that no other thread ever writes to `var`
+ * (no atomicIncr, no atomicSet, no other call to this macro from a different
+ * thread). Concurrent writers cause silent lost updates. Readers on other
+ * threads using atomicGet are fine: they will observe either the pre or
+ * post update value. */
+#define atomicIncrGetSingleWriter(var, delta, newvalue_var) do { \
+    atomicGet((var), (newvalue_var)); \
+    (newvalue_var) += (delta); \
+    atomicSet((var), (newvalue_var)); \
+} while(0)
+
 #endif /* __ATOMIC_VAR_H */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7a48ad2108` → 本草稿移入 `cases/defect/auto-redis-7a48ad2108/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
