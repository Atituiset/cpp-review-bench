# auto-redis-320fe1de5c

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15659 (https://github.com/redis/redis/pull/15659)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2184；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2181,10 +2181,11 @@ int getKeysUsingKeySpecs(struct redisCommand *cmd, robj **argv, int argc, int se
         } else if (spec->find_keys_type == KSPEC_FK_KEYNUM) {
             step = spec->fk.keynum.keystep;
             long long numkeys;
-            if (spec->fk.keynum.keynumidx >= argc)
+            long keynumidx = first + spec->fk.keynum.keynumidx;
+            if (keynumidx >= argc || keynumidx < 0)
                 goto invalid_spec;
 
-            sds keynum_str = argv[first + spec->fk.keynum.keynumidx]->ptr;
+            sds keynum_str = argv[keynumidx]->ptr;
             if (!string2ll(keynum_str,sdslen(keynum_str),&numkeys) || numkeys < 0) {
                 /* Unable to parse the numkeys argument or it was invalid */
                 goto invalid_spec;
@@ -2465,6 +2466,10 @@ int genericGetKeys(int storeKeyOfs, int keyCountOfs, int firstKeyOfs, int keySte
     int i, num;
     keyReference *keys;
 
+    if (keyCountOfs >= argc) {
+        result->numkeys = 0;
+        return 0;
+    }
     num = atoi(argv[keyCountOfs]->ptr);
     /* Sanity check. Don't return any key if the command is going to
      * reply with syntax error. (no input keys). */
@@ -2597,12 +2602,12 @@ int sortGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysResult *
                 i += skiplist[j].skip;
                 break;
             } else if (!strcasecmp(argv[i]->ptr,"store") && i+1 < argc) {
-                /* Note: we don't increment "num" here and continue the loop
-                 * to be sure to process the *last* "STORE" option if multiple
-                 * ones are provided. This is same behavior as SORT. */
+                /* Don't increment "num" so the *last* STORE option wins if
+                 * several are given (same behavior as SORT). */
                 found_store = 1;
                 keys[num].pos = i+1; /* <store-key> */
                 keys[num].flags = CMD_KEY_OW | CMD_KEY_UPDATE;
+                i++; /* Skip the store argument so it isn't re-parsed as an option keyword. */
                 break;
             }
         }
@@ -2667,8 +2672,10 @@ int migrateGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysResul
  * GEORADIUS key x y radius unit [WITHDIST] [WITHHASH] [WITHCOORD] [ASC|DESC]
  *                             [COUNT count] [STORE key|STOREDIST key]
  * GEORADIUSBYMEMBER key member radius unit ... options ...
- * 
- * This command has a fully defined keyspec, so returning flags isn't needed. */
+ *
+ * STORE/STOREDIST keyspecs are marked incomplete because duplicate options
+ * use last-wins semantics (same as georadiusGeneric). ACL and other callers
+ * of getKeysFromCommandWithSpecs fall back here. */
 int georadiusGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysResult *result) {
     int i, num;
     keyReference *keys;
@@ -2697,10 +2704,10 @@ int georadiusGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysRes
 
     /* Add all key positions to keys[] */
     keys[0].pos = 1;
-    keys[0].flags = 0;
-    if(num > 1) {
+    keys[0].flags = CMD_KEY_RO | CMD_KEY_ACCESS;
+    if (num > 1) {
          keys[1].pos = stored_key;
-         keys[1].flags = 0;
+         keys[1].flags = CMD_KEY_OW | CMD_KEY_UPDATE;
     }
     result->numkeys = num;
     return num;
@@ -2709,7 +2716,8 @@ int georadiusGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysRes
 /* XREAD [BLOCK <milliseconds>] [COUNT <count>] [GROUP <groupname> <ttl>]
  *       STREAMS key_1 key_2 ... key_N ID_1 ID_2 ... ID_N
  *
- * This command has a fully defined keyspec, so returning flags isn't needed. */
+ * The keyspec is incomplete, so callers fall back to this function to parse
+ * the options and locate the real STREAMS token. */
 int xreadGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysResult *result) {
     int i, num = 0;
     keyReference *keys;
@@ -2750,7 +2758,7 @@ int xreadGetKeys(struct redisCommand *cmd, robj **argv, int argc, getKeysRe
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-320fe1de5c` → 本草稿移入 `cases/defect/auto-redis-320fe1de5c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
