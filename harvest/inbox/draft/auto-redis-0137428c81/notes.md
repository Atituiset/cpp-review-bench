# auto-redis-0137428c81

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15021 (https://github.com/redis/redis/pull/15021)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 3596；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3589,15 +3589,16 @@ static int parseHashCommandArgs(client *c, HashCommandArgs *args,
                                               &numFields, "Parameter `numFields` should be greater than 0") != C_OK)
                 return C_ERR;
 
-            args->fieldCount = (int)numFields;
             args->firstFieldPos = i + 2;
 
             /* Check bounds - we must have exactly the right number of fields */
-            if (args->firstFieldPos + args->fieldCount > c->argc) {
+            if (numFields > c->argc - args->firstFieldPos) {
                 addReplyError(c, "wrong number of arguments");
                 return C_ERR;
             }
 
+            args->fieldCount = (int)numFields;
+
             /* Skip over the field arguments */
             i = args->firstFieldPos + args->fieldCount - 1;
             continue;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-0137428c81` → 本草稿移入 `cases/defect/auto-redis-0137428c81/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
