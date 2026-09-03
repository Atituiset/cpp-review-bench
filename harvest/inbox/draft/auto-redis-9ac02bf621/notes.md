# auto-redis-9ac02bf621

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14816 (https://github.com/redis/redis/pull/14816)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 16；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1,5 +1,7 @@
 #include "redismodule.h"
+#include <string.h>
 #include <strings.h>
+#include <assert.h>
 int mutable_bool_val, no_prefix_bool, no_prefix_bool2;
 int immutable_bool_val;
 long long longval, no_prefix_longval;
@@ -13,38 +15,37 @@ int flagsval;
  * to point to the config, and they register the configs as such. Note that one could also just
  * use names if they wanted, and store anything in privdata. */
 int getBoolConfigCommand(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
+    assert(strcmp(name, "mutable_bool") == 0 || strcmp(name, "immutable_bool") == 0);
     return (*(int *)privdata);
 }
 
 int setBoolConfigCommand(const char *name, int new, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(err);
+    assert(strcmp(name, "mutable_bool") == 0 || strcmp(name, "immutable_bool") == 0);
     *(int *)privdata = new;
     return REDISMODULE_OK;
 }
 
 long long getNumericConfigCommand(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
+    assert(strcmp(name, "numeric") == 0 || strcmp(name, "memory_numeric") == 0);
     return (*(long long *) privdata);
 }
 
 int setNumericConfigCommand(const char *name, long long new, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(err);
+    assert(strcmp(name, "numeric") == 0 || strcmp(name, "memory_numeric") == 0);
     *(long long *)privdata = new;
     return REDISMODULE_OK;
 }
 
 RedisModuleString *getStringConfigCommand(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "string") == 0);
     return strval;
 }
 int setStringConfigCommand(const char *name, RedisModuleString *new, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
-    REDISMODULE_NOT_USED(err);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "string") == 0);
     size_t len;
     if (!strcasecmp(RedisModule_StringPtrLen(new, &len), "rejectisfreed")) {
         *err = RedisModule_CreateString(NULL, "Cannot set string to 'rejectisfreed'", 36);
@@ -57,29 +58,29 @@ int setStringConfigCommand(const char *name, RedisModuleString *new, void *privd
 }
 
 int getEnumConfigCommand(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "enum") == 0);
     return enumval;
 }
 
 int setEnumConfigCommand(const char *name, int val, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
-    REDISMODULE_NOT_USED(err);
     REDISMODULE_NOT_USED(privdata);
+    REDISMODULE_NOT_USED(err);
+    assert(strcmp(name, "enum") == 0);
     enumval = val;
     return REDISMODULE_OK;
 }
 
 int getFlagsConfigCommand(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "flags") == 0);
     return flagsval;
 }
 
 int setFlagsConfigCommand(const char *name, int val, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(err);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "flags") == 0);
     flagsval = val;
     return REDISMODULE_OK;
 }
@@ -102,34 +103,60 @@ int longlongApplyFunc(RedisModuleCtx *ctx, void *privdata, RedisModuleString **e
         return REDISMODULE_ERR;
     }
     return REDISMODULE_OK;
-}
+}    
 
 RedisModuleString *getStringConfigUnprefix(const char *name, void *privdata) {
-    REDISMODULE_NOT_USED(name);
     REDISMODULE_NOT_USED(privdata);
+    assert(strcmp(name, "unprefix-string") == 0 || strcmp(name, "unprefix.string-alias") == 0);
     return strval2;
 }
 
 int setStringConfigUnprefix(const char *name, RedisModuleString *new, void *privdata, RedisModuleString **err) {
-    REDISMODULE_NOT_USED(name);
-    REDISMODULE_NOT_USED(err);
     REDISMODULE_NOT_USED(privdata);
+    REDISMODULE_NOT_USED(err);
+    assert(strcm
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-9ac02bf621` → 本草稿移入 `cases/defect/auto-redis-9ac02bf621/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
