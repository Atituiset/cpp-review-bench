# auto-redis-7043e93011

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15466 (https://github.com/redis/redis/pull/15466)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 117；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -85,17 +85,20 @@ ConnectionType *connTypeOfCluster(void) {
  * -------------------------------------------------------------------------- */
 
 /* Generates a DUMP-format representation of the object 'o', adding it to the
- * io stream pointed by 'rio'. This function can't fail. */
-void createDumpPayload(rio *payload, robj *o, robj *key, int dbid, int skip_checksum) {
+ * io stream pointed by 'rio'. Flags can omit the checksum or key metadata.
+ * This function can't fail. */
+void createDumpPayload(rio *payload, robj *o, robj *key, int dbid, int flags) {
     unsigned char buf[2];
     uint64_t crc = 0;
 
     /* Serialize the object in an RDB-like format. It consist of an object type
      * byte followed by the serialized object. This is understood by RESTORE. */
     rioInitWithBuffer(payload,sdsempty());
 
-    /* Save key metadata if present without (handles TTL separately via command args) */
-    if (getModuleMetaBits(o->metabits))
+    /* Save key metadata if present (TTL is handled separately via command
+     * args). AOF RESTORE payloads omit it because AOF rewrite handles module
+     * metadata separately through keyMetaOnAof(). */
+    if (!(flags & DUMP_PAYLOAD_SKIP_KEY_META) && getModuleMetaBits(o->metabits))
         serverAssert(rdbSaveKeyMetadata(payload, key, o, dbid) != -1);
     serverAssert(rdbSaveObjectType(payload,o));
     serverAssert(rdbSaveObject(payload,o,key,dbid));
@@ -114,7 +117,7 @@ void createDumpPayload(rio *payload, robj *o, robj *key, int dbid, int skip_chec
 
     /* If crc checksum is disabled, crc is set to 0 and no checksum validation
      * will be performed on RESTORE. */
-    if (!skip_checksum) {
+    if (!(flags & DUMP_PAYLOAD_SKIP_CHECKSUM)) {
         /* CRC64 */
         crc = crc64(0,(unsigned char*)payload->io.buffer.ptr,
                     sdslen(payload->io.buffer.ptr));
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7043e93011` → 本草稿移入 `cases/defect/auto-redis-7043e93011/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
