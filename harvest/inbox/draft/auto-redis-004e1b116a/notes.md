# auto-redis-004e1b116a

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15182 (https://github.com/redis/redis/pull/15182)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 32；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -29,12 +29,12 @@ slowlogEntry *slowlogCreateEntry(client *c, robj **argv, int argc, long long dur
     slowlogEntry *se = zmalloc(sizeof(*se));
     int j, slargc = argc;
 
-    if (slargc > SLOWLOG_ENTRY_MAX_ARGC) slargc = SLOWLOG_ENTRY_MAX_ARGC;
+    if (slargc > server.slowlog_max_argc) slargc = server.slowlog_max_argc;
     se->argc = slargc;
     se->argv = zmalloc(sizeof(robj*)*slargc);
     for (j = 0; j < slargc; j++) {
         /* Logging too many arguments is a useless memory waste, so we stop
-         * at SLOWLOG_ENTRY_MAX_ARGC, but use the last argument to specify
+         * at server.slowlog_max_argc, but use the last argument to specify
          * how many remaining arguments there were in the original command. */
         if (slargc != argc && j == slargc-1) {
             se->argv[j] = createObject(OBJ_STRING,
@@ -44,13 +44,13 @@ slowlogEntry *slowlogCreateEntry(client *c, robj **argv, int argc, long long dur
             /* Trim too long strings as well... */
             if (argv[j]->type == OBJ_STRING &&
                 sdsEncodedObject(argv[j]) &&
-                sdslen(argv[j]->ptr) > SLOWLOG_ENTRY_MAX_STRING)
+                sdslen(argv[j]->ptr) > server.slowlog_max_string_len)
             {
-                sds s = sdsnewlen(argv[j]->ptr, SLOWLOG_ENTRY_MAX_STRING);
+                sds s = sdsnewlen(argv[j]->ptr, server.slowlog_max_string_len);
 
                 s = sdscatprintf(s,"... (%lu more bytes)",
                     (unsigned long)
-                    sdslen(argv[j]->ptr) - SLOWLOG_ENTRY_MAX_STRING);
+                    sdslen(argv[j]->ptr) - server.slowlog_max_string_len);
                 se->argv[j] = createObject(OBJ_STRING,s);
             } else if (argv[j]->refcount == OBJ_SHARED_REFCOUNT) {
                 se->argv[j] = argv[j];
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-004e1b116a` → 本草稿移入 `cases/defect/auto-redis-004e1b116a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
