# auto-redis-fb488aba27

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14927 (https://github.com/redis/redis/pull/14927)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 33；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -30,14 +30,17 @@ extern int __test_num;
 
 #define test_cond(descr,_c) do { \
     __test_num++; printf("%d - %s: ", __test_num, descr); \
-    if(_c) printf("PASSED\n"); else {printf("FAILED\n"); __failed_tests++;} \
+    if(_c) printf("\033[32mPASSED\033[0m\n"); else {printf("\033[31mFAILED\033[0m\n"); __failed_tests++;} \
 } while(0)
 #define test_report() do { \
-    printf("%d tests, %d passed, %d failed\n", __test_num, \
-                    __test_num-__failed_tests, __failed_tests); \
     if (__failed_tests) { \
-        printf("=== WARNING === We have failed tests here...\n"); \
+        printf("  Tests:       %d passed, \033[31m%d failed\033[0m, %d total\n", \
+                        __test_num-__failed_tests, __failed_tests, __test_num); \
+        printf("\033[31m=== WARNING === We have failed tests here...\033[0m\n"); \
         exit(1); \
+    } else { \
+        printf("  Tests:       \033[32m%d passed\033[0m, %d failed, %d total\n", \
+                        __test_num-__failed_tests, __failed_tests, __test_num); \
     } \
 } while(0)
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-fb488aba27` → 本草稿移入 `cases/defect/auto-redis-fb488aba27/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
