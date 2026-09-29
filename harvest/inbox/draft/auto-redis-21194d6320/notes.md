# auto-redis-21194d6320

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15804](https://github.com/redis/redis/pull/15804) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 52 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15804 (https://github.com/redis/redis/pull/15804)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 387；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15804 Redis 8.10.2 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -25,6 +25,7 @@
 #define NUM_TEST_ITERATIONS 100000
 #define NUM_CORRUPTION_TESTS 10000
 #define NUM_BOUNDARY_TESTS 10000
+#define NUM_RECURSION_DEPTH_TESTS 8  /* 6 depth levels + 2 validation tests */
 
 /* Test state tracking */
 static char *safe_page = NULL;       /* Start of readable/writable page */
@@ -35,6 +36,7 @@ static int tests_passed = 0;
 static int tests_failed = 0;
 static int corruptions_passed = 0;
 static int boundary_tests_passed = 0;
+static int recursion_tests_passed = 0;
 
 /* Test metadata for tracking */
 typedef struct {
@@ -53,6 +55,7 @@ void cleanup_test_memory(void);
 void run_normal_tests(void);
 void run_corruption_tests(void);
 void run_boundary_tests(void);
+void run_recursion_depth_tests(void);
 void print_test_summary(void);
 
 /* Signal handler for segmentation violations */
@@ -368,12 +371,127 @@ void run_boundary_tests(void) {
     }
 }
 
+/* Run tests for recursion depth protection */
+void run_recursion_depth_tests(void) {
+    printf("Running recursion depth protection tests...\n");
+
+    /* Test 1: Deeply nested arrays that should be rejected */
+    /* Create a JSON with nested arrays beyond MAX_RECURSION_DEPTH_LIMIT (1000) */
+    const int test_depths[] = {500, 999, 1000, 1001, 1500, 2000};
+    const int num_depth_tests = sizeof(test_depths) / sizeof(test_depths[0]);
+
+    for (int t = 0; t < num_depth_tests; t++) {
+        int depth = test_depths[t];
+
+        /* Build deeply nested array JSON: [[[[...]]]] with a field at the top level */
+        /* Format: {"field": [[[[...]]]]} */
+        size_t json_size = depth * 2 + 100; /* Each level adds '[' and ']' */
+        char *json = malloc(json_size);
+        if (!json) {
+            perror("malloc");
+            exit(EXIT_FAILURE);
+        }
+
+        size_t pos = 0;
+        pos += snprintf(json + pos, json_size - pos, "{\"testfield\": ");
+
+        /* Add opening brackets */
+        for (int i = 0; i < depth; i++) {
+            if (pos >= json_size - 10) break; /* Safety check */
+            json[pos++] = '[';
+        }
+
+        /* Add a simple value at the deepest level */
+        pos += snprintf(json + pos, json_size - pos, "42");
+
+        /* Add closing brackets */
+        for (int i = 0; i < depth; i++) {
+            if (pos >= json_size - 10) break; /* Safety check */
+            json[pos++] = ']';
+        }
+
+        pos += snprintf(json + pos, json_size - pos, "}");
+
+        /* Try to extract the field */
+        const char *field = "testfield";
+        exprtoken *token = safe_extract_field(json, pos, field, strlen(field));
+
+        /* For depths >= 1000, we expect NULL (recursion limit hit) */
+        /* For depths < 1000, we expect a valid token */
+        if (depth >= 1000) {
+            if (token == NULL) {
+                printf("PASS: Depth %d correctly rejected (recursion limit)\n", depth);
+                recursion_tests_passed++;
+            } else {
+                printf("FAIL: Expected NULL for depth %d but got a token\n", depth);
+                exprTokenRelease(token);
+                tests_failed++;
+            }
+        } else {
+            if (token != NULL) {
+                printf("PASS: Depth %d correctly accepted\n", depth);
+                exprTokenRelease(token);
+                recursion_tests_passed++;
+            } else {
+                printf("FAIL: Expected valid token for depth %d but got NULL\n", depth);
+                tests_failed++;
+            }
+        }
+
+        free(json);
+    }
+
+    /* Test 2: Verify that reasonable nesting still works */
+    const char *valid_nested = "{\"data\": [[1, 2], [3, 4], [5, 6]]}";
+    exprtoken *token = safe_extract_field(valid_nested, strlen(valid_nested), "data", 4);
+    if (token != NULL) {
+        printf("PASS: Valid nested array accepted\n");
+        exprTokenRelease(token);
+        recursion_tests_passed++;
+    } else {
+        printf("FAIL: Valid nested array was rejected\n");
+      
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`exit`
- 外部函数：`exprTokenRelease`
- 外部函数：`field`
- 外部函数：`free`
- 外部函数：`jsonExtractField`
- 外部函数：`malloc`
- 外部函数：`memcpy`
- 外部函数：`munmap`
- 外部函数：`perror`
- 外部函数：`printf`
- 外部函数：`rand`
- 外部函数：`setjmp`
- 外部函数：`snprintf`
- 外部函数：`strlen`
- 大写宏：`EXIT_FAILURE`
- 大写宏：`FASTJSON`
- 大写宏：`JSON`
- 大写宏：`NULL`
- 大写宏：`PARSER`
- 大写宏：`RAND_MAX`
- 大写宏：`SUMMARY`
- 大写宏：`TEST`
- 外部类型：`Add`
- 外部类型：`Array`
- 外部类型：`Boolean`
- 外部类型：`Boundary`
- 外部类型：`Buffer`
- 外部类型：`Check`
- 外部类型：`Cleanup`
- 外部类型：`Close`
- 外部类型：`Corrupt`
- 外部类型：`Corruption`
- 外部类型：`Ensure`
- 外部类型：`Entry`
- 外部类型：`Failed`
- 外部类型：`Generate`
- 外部类型：`Keep`
- 外部类型：`Make`
- 外部类型：`Normal`
- 外部类型：`Null`
- 外部类型：`Number`
- 外部类型：`Occasionally`
- 外部类型：`Place`
- 外部类型：`Print`
- 外部类型：`Random`
- 外部类型：`Return`
- 外部类型：`Running`
- 外部类型：`Seed`
- 外部类型：`Signal`
- 外部类型：`Sometimes`
- 外部类型：`Start`
- 外部类型：`Starting`
- 外部类型：`String`
- 外部类型：`Test`
- 外部类型：`This`
- 外部类型：`Too`
- 外部类型：`Truncate`
- 外部类型：`Use`
- 外部类型：`We`
- 外部类型：`Which`
- 外部类型：`size_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-redis-21194d6320` → 本草稿移入 `cases/defect/auto-redis-21194d6320/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
