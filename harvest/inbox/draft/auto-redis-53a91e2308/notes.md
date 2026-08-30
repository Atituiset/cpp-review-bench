# auto-redis-53a91e2308

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15256 (https://github.com/redis/redis/pull/15256)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 545；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -542,7 +542,7 @@ static rax *ebConvertListToRax(eItem listHead, EbucketsType *type) {
     /* Use min expire-time for the first segment in rax */
     unsigned char raxKey[EB_KEY_SIZE];
     bucketKey2RaxKey(bucketKey, raxKey);
-    rax *rax = raxNewWithMetadata(sizeof(uint64_t), NULL);
+    rax *rax = raxNewEx(sizeof(uint64_t), NULL, EB_KEY_SIZE);
     *ebRaxNumItems(rax) = EB_LIST_MAX_ITEMS;
     raxInsert(rax, raxKey, EB_KEY_SIZE, firstSegHdr, NULL);
     return rax;
@@ -1879,7 +1879,7 @@ void ebDefragRaxBucket(EbucketsType *type, raxIterator *ri,
         if (newSegHdr) {
             if (currentSegHdr == ri->data) {
                 /* If it's the first segment, update the rax data pointer. */
-                raxSetData(ri->node, ri->data=newSegHdr);
+                raxIteratorSetData(ri, newSegHdr);
                 firstSegHdr = newSegHdr;
             } else {
                 /* For non-first segments, update the previous segment's next
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-53a91e2308` → 本草稿移入 `cases/defect/auto-redis-53a91e2308/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
