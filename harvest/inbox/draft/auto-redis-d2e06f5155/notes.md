# auto-redis-d2e06f5155

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14690 (https://github.com/redis/redis/pull/14690)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 898；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -895,7 +895,7 @@ int TestBasics(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
     if (!TestAssertStringReply(ctx,RedisModule_CallReplyArrayElement(reply, 1),"1234",4)) goto fail;
 
     T("foo", "E");
-    if (!TestAssertErrorReply(ctx,reply,"ERR unknown command 'foo', with args beginning with: ",53)) goto fail;
+    if (!TestAssertErrorReply(ctx,reply,"ERR unknown command 'foo'",25)) goto fail;
 
     T("set", "Ec", "x");
     if (!TestAssertErrorReply(ctx,reply,"ERR wrong number of arguments for 'set' command",47)) goto fail;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-d2e06f5155` → 本草稿移入 `cases/defect/auto-redis-d2e06f5155/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
