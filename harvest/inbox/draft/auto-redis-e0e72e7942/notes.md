# auto-redis-e0e72e7942

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15255 (https://github.com/redis/redis/pull/15255)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 327；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -324,11 +324,18 @@ void parseRedisUri(const char *uri, const char* tool_name, cliConnInfo *connInfo
     /* Extract user info. */
     if ((userinfo = strchr(curr,'@'))) {
         if ((username = strchr(curr, ':')) && username < userinfo) {
-            connInfo->user = percentDecode(curr, username - curr);
+            /* Free any value previously set via --user / -a (later
+             * parameters override earlier ones) and use NULL for an
+             * explicitly empty component, so cliAuth() falls back to the
+             * legacy single-argument AUTH (empty username) or skips AUTH
+             * entirely (empty password) instead of sending an empty ACL
+             * component, which the server rejects. */
+            sdsfree(connInfo->user);
+            connInfo->user = (username > curr) ? percentDecode(curr, username - curr) : NULL;
             curr = username + 1;
         }
-
-        connInfo->auth = percentDecode(curr, userinfo - curr);
+        sdsfree(connInfo->auth);
+        connInfo->auth = (userinfo > curr) ? percentDecode(curr, userinfo - curr) : NULL;
         curr = userinfo + 1;
     }
     if (curr == end) return;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-e0e72e7942` → 本草稿移入 `cases/defect/auto-redis-e0e72e7942/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
