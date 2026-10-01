# auto-redis-e3c24918f3

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14987 (https://github.com/redis/redis/pull/14987)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 335；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -332,7 +332,7 @@ void emptyDbAsync(redisDb *db) {
     db->keys = kvstoreCreate(&kvstoreExType, &dbDictType, slot_count_bits, flags);
     db->expires = kvstoreCreate(&kvstoreBaseType, &dbExpiresDictType, slot_count_bits, flags);
     db->subexpires = estoreCreate(&subexpiresBucketsType, slot_count_bits);
-    db->stream_idmp_keys = dictCreate(&objectKeyPointerValueDictType);
+    db->stream_idmp_keys = dictCreate(&objectKeyNoValueDictType);
     protectClientReplyObjects(); /* Protect client reply objects before async free. */
     emptyDbDataAsync(oldkeys, oldexpires, oldsubexpires, old_stream_idmp_keys, NULL);
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e3c24918f3` → 本草稿移入 `cases/defect/auto-redis-e3c24918f3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
