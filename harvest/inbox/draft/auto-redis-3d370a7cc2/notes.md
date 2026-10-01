# auto-redis-3d370a7cc2

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15045 (https://github.com/redis/redis/pull/15045)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 74；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -9,6 +9,7 @@
 
 #include "server.h"
 #include "xxhash.h"
+#include <float.h>
 #include <math.h> /* isnan(), isinf() */
 
 /* XXH3 64-bit hash produces 16 hex characters when formatted */
@@ -71,7 +72,7 @@ static int checkStringLength(client *c, long long size, long long append) {
 #define OBJ_SET_IFDNE (1<<12)      /* Set if current digest does not equal match digest */
 
 /* Forward declaration */
-static int getExpireMillisecondsOrReply(client *c, robj *expire, int flags, int unit, long long *milliseconds);
+static int getExpireMillisecondsOrReply(client *c, robj *expire, int relative_ttl, int unit, long long *milliseconds);
 
 /* Generic SET command family (SET, SETEX, PSETEX, SETNX)
  *
@@ -88,8 +89,9 @@ void setGenericCommand(client *c, int flags, robj *key, robj **valref, robj *exp
     long long milliseconds = 0; /* initialized to avoid any harmless warning */
     int found = 0;
     int setkey_flags = 0;
+    int relative_ttl = (flags & (OBJ_EX|OBJ_PX)) != 0; /* EX/PX are relative; EXAT/PXAT are absolute. */
 
-    if (expire && getExpireMillisecondsOrReply(c, expire, flags, unit, &milliseconds) != C_OK) {
+    if (expire && getExpireMillisecondsOrReply(c, expire, relative_ttl, unit, &milliseconds) != C_OK) {
         return;
     }
 
@@ -231,18 +233,19 @@ void setGenericCommand(client *c, int flags, robj *key, robj **valref, robj *exp
 }
 
 /*
- * Extract the `expire` argument of a given GET/SET command as an absolute timestamp in milliseconds.
+ * Extract the `expire` argument of a given command as an absolute timestamp in milliseconds.
  *
  * "client" is the client that sent the `expire` argument.
  * "expire" is the `expire` argument to be extracted.
- * "flags" represents the behavior of the command (e.g. PX or EX).
+ * "relative_ttl" is true when the value is a relative TTL (EX/PX),
+ *                false when it is an absolute timestamp (EXAT/PXAT).
  * "unit" is the original unit of the given `expire` argument (e.g. UNIT_SECONDS).
  * "milliseconds" is output argument.
  *
  * If return C_OK, "milliseconds" output argument will be set to the resulting absolute timestamp.
  * If return C_ERR, an error reply has been added to the given client.
  */
-static int getExpireMillisecondsOrReply(client *c, robj *expire, int flags, int unit, long long *milliseconds) {
+static int getExpireMillisecondsOrReply(client *c, robj *expire, int relative_ttl, int unit, long long *milliseconds) {
     int ret = getLongLongFromObjectOrReply(c, expire, milliseconds, NULL);
     if (ret != C_OK) {
         return ret;
@@ -256,7 +259,7 @@ static int getExpireMillisecondsOrReply(client *c, robj *expire, int flags, int
 
     if (unit == UNIT_SECONDS) *milliseconds *= 1000;
 
-    if ((flags & OBJ_PX) || (flags & OBJ_EX)) {
+    if (relative_ttl) {
         *milliseconds += commandTimeSnapshot();
     }
 
@@ -515,7 +518,8 @@ void getexCommand(client *c) {
 
     /* Validate the expiration time value first */
     long long milliseconds = 0;
-    if (args.expire && getExpireMillisecondsOrReply(c, args.expire, args.flags, args.unit, &milliseconds) != C_OK) {
+    int relative_ttl = (args.flags & (OBJ_EX|OBJ_PX)) != 0; /* EX/PX are relative; EXAT/PXAT are absolute. */
+    if (args.expire && getExpireMillisecondsOrReply(c, args.expire, relative_ttl, args.unit, &milliseconds) != C_OK) {
         return;
     }
 
@@ -767,7 +771,8 @@ void msetexCommand(client *c) {
 
     /* Validate the expiration time value first */
     long long milliseconds = 0;
-    if (args.expire && getExpireMillisecondsOrReply(c, args.expire, args.flags, args.unit, &milliseconds) != C_OK) {
+    int relative_ttl = (args.flags & (OBJ_EX|OBJ_PX)) != 0; /* EX/PX are relative; EXAT/PXAT are absolute. */
+    if (args.expire && getExpireMillisecondsOrReply(c, args.expire, relative_ttl, args.unit, &milliseconds) != C_OK) {
         return;
     }
 
@@ -923,6 +928,405 @@ void incrbyfloatCommand(client *c) {
     rewriteClientCommandArgument(c,3,shared.keepttl);
 }
 

```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-3d370a7cc2` → 本草稿移入 `cases/defect/auto-redis-3d370a7cc2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
