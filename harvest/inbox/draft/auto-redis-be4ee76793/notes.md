# auto-redis-be4ee76793

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14721 (https://github.com/redis/redis/pull/14721)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 12（原始 PR diff 行 11138；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3585,6 +3585,7 @@ static void repl(void) {
             if (strcasecmp(argv[0],"quit") == 0 ||
                 strcasecmp(argv[0],"exit") == 0)
             {
+                redisFree(context);
                 exit(0);
             } else if (argv[0][0] == ':') {
                 cliSetPreferences(argv,argc,1);
@@ -3644,6 +3645,8 @@ static void repl(void) {
         /* linenoise() returns malloc-ed lines like readline() */
         linenoiseFree(line);
     }
+
+    redisFree(context);
     exit(0);
 }
 
@@ -11135,9 +11138,13 @@ int main(int argc, char **argv) {
     /* Otherwise, we have some arguments to execute */
     if (config.eval) {
         if (cliConnect(0) != REDIS_OK) exit(1);
-        return evalMode(argc,argv);
+        int res = evalMode(argc,argv);
+        redisFree(context);
+        return res;
     } else {
         cliConnect(CC_QUIET);
-        return noninteractive(argc,argv);
+        int res = noninteractive(argc,argv);
+        redisFree(context);
+        return res;
     }
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-be4ee76793` → 本草稿移入 `cases/defect/auto-redis-be4ee76793/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
