# auto-redis-ba5b998a07

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15561 (https://github.com/redis/redis/pull/15561)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 438；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -431,21 +431,21 @@ void hotkeysCommand(client *c) {
                         return;
                     }
 
-                    /* Check for duplicate slot */
-                    for (int k = 0; k < i; k++) {
-                        if (temp_slots[k] == slot_val) {
-                            addReplyError(c, "duplicate slot number");
-                            zfree(temp_slots);
-                            return;
-                        }
-                    }
-
                     temp_slots[i] = (int)slot_val;
                 }
 
                 /* Sort the slots array */
                 qsort(temp_slots, slots_count, sizeof(int), slotCompare);
 
+                /* Check for duplicates */
+                for (int i = 1; i < slots_count; ++i) {
+                    if (temp_slots[i] == temp_slots[i-1]) {
+                        addReplyError(c, "duplicate slot number");
+                        zfree(temp_slots);
+                        return;
+                    }
+                }
+
                 /* Build slotRangeArray from sorted slots */
                 for (int i = 0; i < slots_count; i++) {
                     slots = slotRangeArrayAppend(slots, temp_slots[i]);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-ba5b998a07` → 本草稿移入 `cases/defect/auto-redis-ba5b998a07/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
