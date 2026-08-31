# auto-redis-3209173406

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15604 (https://github.com/redis/redis/pull/15604)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -4007,6 +4007,7 @@ typedef struct hashTemplateRegistry {
 typedef struct hashTemplateArray {
     uint64_t tmpl_id;    /* Template id; resolve via hashTemplateGetById. */
     unsigned long long field_count;
+    size_t alloc_size;   /* Usable struct/array bytes plus value SDS alloc sizes. */
     sds values[];       /* Flexible array: values in template field order. */
 } hashTemplateArray;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-3209173406` → 本草稿移入 `cases/defect/auto-redis-3209173406/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
