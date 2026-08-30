# auto-redis-1c99e4f57b

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15673 (https://github.com/redis/redis/pull/15673)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 12031；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -383,6 +383,8 @@ typedef struct RedisModuleCommandFilterCtx {
     RedisModuleString **argv;
     int argv_len;
     int argc;
+    int argv_changed; /* Set when a filter modified argv, so the caller knows
+                       * it must refresh what it cached about the command. */
     client *c;
 } RedisModuleCommandFilterCtx;
 
@@ -11999,6 +12001,7 @@ void moduleCallCommandFilters(client *c) {
         .argv = c->argv,
         .argv_len = c->argv_len,
         .argc = c->argc,
+        .argv_changed = 0,
         .c = c
     };
 
@@ -12014,6 +12017,9 @@ void moduleCallCommandFilters(client *c) {
         f->callback(&filter);
     }
 
+    /* Nothing was modified by the filters, there's nothing to refresh. */
+    if (!filter.argv_changed) return;
+
     /* If the filter sets a new command, including command or subcommand,
      * the command looked up will be invalid. */
     c->lookedcmd = NULL;
@@ -12022,19 +12028,19 @@ void moduleCallCommandFilters(client *c) {
     c->argv_len = filter.argv_len;
     c->argc = filter.argc;
 
-    /* Update pending command if it exists. */
+    /* Everything the pending command derived from the old argv (command, keys,
+     * slot, read error) is stale now, so preprocess it again. */
     pendingCommand *pcmd = c->current_pending_cmd;
     if (pcmd) {
         pcmd->argv = filter.argv;
         pcmd->argc = filter.argc;
         pcmd->argv_len = filter.argv_len;
-        pcmd->cmd = NULL;
-        pcmd->slot = INVALID_CLUSTER_SLOT;
-        pcmd->flags = 0;
+        preprocessCommand(c, pcmd);
 
-        /* Reset keys result */
-        getKeysFreeResult(&pcmd->keys_result);
-        pcmd->keys_result = (getKeysResult)GETKEYS_RESULT_INIT;
+        /* Keep the client fields we populated from the pending command in sync. */
+        c->lookedcmd = pcmd->cmd;
+        c->slot = pcmd->slot;
+        c->read_error = pcmd->read_error;
     }
 }
 
@@ -12075,6 +12081,7 @@ int RM_CommandFilterArgInsert(RedisModuleCommandFilterCtx *fctx, int pos, RedisM
     }
     fctx->argv[pos] = arg;
     fctx->argc++;
+    fctx->argv_changed = 1;
 
     return REDISMODULE_OK;
 }
@@ -12090,6 +12097,7 @@ int RM_CommandFilterArgReplace(RedisModuleCommandFilterCtx *fctx, int pos, Redis
 
     decrRefCount(fctx->argv[pos]);
     fctx->argv[pos] = arg;
+    fctx->argv_changed = 1;
 
     return REDISMODULE_OK;
 }
@@ -12107,6 +12115,7 @@ int RM_CommandFilterArgDelete(RedisModuleCommandFilterCtx *fctx, int pos)
         fctx->argv[i] = fctx->argv[i+1];
     }
     fctx->argc--;
+    fctx->argv_changed = 1;
 
     return REDISMODULE_OK;
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-1c99e4f57b` → 本草稿移入 `cases/defect/auto-redis-1c99e4f57b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
