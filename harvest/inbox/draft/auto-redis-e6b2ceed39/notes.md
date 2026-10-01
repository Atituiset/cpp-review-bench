# auto-redis-e6b2ceed39

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14877 (https://github.com/redis/redis/pull/14877)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 888；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -885,8 +885,7 @@ void setbitCommand(client *c) {
          * update the keysizes histogram. Otherwise, the histogram already 
          * updated in lookupStringForBitCommand() by calling dbAdd(). */
         if ((strOldSize > 0) && (strGrowSize != 0))
-            updateKeysizesHist(c->db, getKeySlot(c->argv[1]->ptr), OBJ_STRING, 
-                               strOldSize, strOldSize + strGrowSize);
+            updateKeysizesHist(c->db, OBJ_STRING, strOldSize, strOldSize + strGrowSize);
     }
 
     /* Return original value. */
@@ -2108,8 +2107,7 @@ void bitfieldGeneric(client *c, int flags) {
          * update the keysizes histogram. Otherwise, the histogram already 
          * updated in lookupStringForBitCommand() by calling dbAdd(). */
         if ((strOldSize > 0) && (strGrowSize != 0))
-            updateKeysizesHist(c->db, getKeySlot(c->argv[1]->ptr), OBJ_STRING,
-                               strOldSize, strOldSize + strGrowSize);
+            updateKeysizesHist(c->db, OBJ_STRING, strOldSize, strOldSize + strGrowSize);
         
         keyModified(c,c->db,c->argv[1],o,1);
         notifyKeyspaceEvent(NOTIFY_STRING,"setbit",c->argv[1],c->db->id);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e6b2ceed39` → 本草稿移入 `cases/defect/auto-redis-e6b2ceed39/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
