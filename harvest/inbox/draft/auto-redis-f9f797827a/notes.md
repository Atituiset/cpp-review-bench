# auto-redis-f9f797827a

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15114 (https://github.com/redis/redis/pull/15114)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 221；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -212,21 +212,16 @@ void setGenericCommand(client *c, int flags, robj *key, robj **valref, robj *exp
 
     /* Propagate without the GET argument (Isn't needed if we had expire since in that case we completely re-written the command argv) */
     if ((flags & OBJ_SET_GET) && !expire) {
-        int argc = 0;
-        int j;
-        robj **argv = zmalloc((c->argc-1)*sizeof(robj*));
-        for (j=0; j < c->argc; j++) {
+        for (int j = c->argc - 1; j >= 3; j--) {
             char *a = c->argv[j]->ptr;
             /* Skip GET which may be repeated multiple times. */
-            if (j >= 3 &&
-                (a[0] == 'g' || a[0] == 'G') &&
+            if ((a[0] == 'g' || a[0] == 'G') &&
                 (a[1] == 'e' || a[1] == 'E') &&
                 (a[2] == 't' || a[2] == 'T') && a[3] == '\0')
-                continue;
-            argv[argc++] = c->argv[j];
-            incrRefCount(c->argv[j]);
+            {
+                rewriteClientCommandArgument(c, j, NULL);
+            }
         }
-        replaceClientCommandVector(c, argc, argv);
     }
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-f9f797827a` → 本草稿移入 `cases/defect/auto-redis-f9f797827a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
