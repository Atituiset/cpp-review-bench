# auto-redis-c920dd0968

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15433 (https://github.com/redis/redis/pull/15433)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 733；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -721,16 +721,22 @@ int getBitOffsetFromArgument(client *c, robj *o, uint64_t *offset, int hash, int
     /* Handle #<offset> form. */
     if (p[0] == '#' && hash && bits > 0) usehash = 1;
 
-    if (string2ll(p+usehash,plen-usehash,&loffset) == 0) {
+    if (string2ll(p+usehash,plen-usehash,&loffset) == 0 || loffset < 0) {
         addReplyError(c,err);
         return C_ERR;
     }
 
     /* Adjust the offset by 'bits' for #<offset> form. */
-    if (usehash) loffset *= bits;
+    if (usehash) {
+        if (loffset > LLONG_MAX / bits) {
+            addReplyError(c,err);
+            return C_ERR;
+        }
+        loffset *= bits;
+    }
 
     /* Limit offset to server.proto_max_bulk_len (512MB in bytes by default) */
-    if (loffset < 0 || (!mustObeyClient(c) && (loffset >> 3) >= server.proto_max_bulk_len))
+    if (!mustObeyClient(c) && (loffset >> 3) >= server.proto_max_bulk_len)
     {
         addReplyError(c,err);
         return C_ERR;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-c920dd0968` → 本草稿移入 `cases/defect/auto-redis-c920dd0968/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
