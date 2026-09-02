# auto-curl-7983968c53

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#8049](https://github.com/curl/curl/pull/8049) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 60（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8049 (https://github.com/curl/curl/pull/8049)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 133；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 8049 urlapi: provide more detailed return codes :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -129,17 +129,21 @@ struct querycase {
 };
 
 static const struct testcase get_parts_list[] ={
+  {"https://user:password@example.net/get?this=and#but frag then", "",
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_FRAGMENT},
   {"https://user:password@example.net/get?this=and what", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_QUERY},
   {"https://user:password@example.net/ge t?this=and-what", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_PATH},
   {"https://user:pass word@example.net/get?this=and-what", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_PASSWORD},
   {"https://u ser:password@example.net/get?this=and-what", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_USER},
+  {"imap://user:pass;opt ion@server/path", "",
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_LOGIN},
   /* no space allowed in scheme */
   {"htt ps://user:password@example.net/get?this=and-what", "",
-   CURLU_NON_SUPPORT_SCHEME|CURLU_ALLOW_SPACE, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_NON_SUPPORT_SCHEME|CURLU_ALLOW_SPACE, 0, CURLUE_BAD_SCHEME},
   {"https://user:password@example.net/get?this=and what",
    "https | user | password | [13] | example.net | [15] | /get | "
    "this=and what | [17]",
@@ -213,9 +217,9 @@ static const struct testcase get_parts_list[] ={
    "com/color/?green#no-red",
    CURLU_DEFAULT_SCHEME, 0, CURLUE_OK },
   {"http://[ab.be:1]/x", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_IPV6},
   {"http://[ab.be]/x", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_IPV6},
   /* URL without host name */
   {"http://a:b@/x", "",
    CURLU_DEFAULT_SCHEME, 0, CURLUE_NO_HOST},
@@ -254,7 +258,7 @@ static const struct testcase get_parts_list[] ={
    0, 0, CURLUE_OK},
   {"file://hello.html",
    "",
-   0, 0, CURLUE_MALFORMED_INPUT},
+   0, 0, CURLUE_BAD_FILE_URL},
   {"http://HO0_-st/",
    "http | [11] | [12] | [13] | HO0_-st | [15] | / | [16] | [17]",
    0, 0, CURLUE_OK},
@@ -272,7 +276,7 @@ static const struct testcase get_parts_list[] ={
    0, 0, CURLUE_OK},
   {"file:/",
    "file | [11] | [12] | [13] | [14] | [15] | | [16] | [17]",
-   0, 0, CURLUE_MALFORMED_INPUT},
+   0, 0, CURLUE_BAD_FILE_URL},
   {"file://127.0.0.1/hello.html",
    "file | [11] | [12] | [13] | [14] | [15] | /hello.html | [16] | [17]",
    0, 0, CURLUE_OK},
@@ -295,9 +299,9 @@ static const struct testcase get_parts_list[] ={
    "https | [11] | [12] | [13] | 127abc.com | [15] | / | [16] | [17]",
    CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
   {"https:// example.com?check", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_HOSTNAME},
   {"https://e x a m p l e.com?check", "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_HOSTNAME},
   {"https://example.com?check",
    "https | [11] | [12] | [13] | example.com | [15] | / | check | [17]",
    CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
@@ -357,7 +361,7 @@ static const struct testcase get_parts_list[] ={
    CURLU_DEFAULT_SCHEME, 0, CURLUE_OK},
   {"http:////user:password@example.com:1234/path/html?query=name#anchor",
    "",
-   CURLU_DEFAULT_SCHEME, 0, CURLUE_MALFORMED_INPUT},
+   CURLU_DEFAULT_SCHEME, 0, CURLUE_BAD_SLASHES},
   {NULL, NULL, 0, 0, CURLUE_OK},
 };
 
@@ -367,8 +371,8 @@ static const struct urltestcase get_url_list[] = {
   {"https://h%c", "https://h%25c/", 0, 0, CURLUE_OK},
   {"https://%%%%%%", "https://%25%25%25%25%25%25/", 0, 0, CURLUE_OK},
   {"https://%41", "https://A/", 0, 0, CURLUE_OK},
-  {"https://%20", "", 0, 0, CURLUE_MALFORMED_INPUT},
-  {"https://%41%0d", "", 0, 0, CURLUE_MALFORMED_INPUT},
+  {"https://%20", "", 0, 0, CURLUE_BAD_HOSTNAME},
+  {"https://%41%0d", "", 0, 0, CURLUE_BAD_HOSTNAME},
   {"https://%25", "h
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA`
- 大写宏：`C0_`
- 大写宏：`CURLUE_BAD_PORT_NUMBER`
- 大写宏：`CURLUE_MALFORMED_INPUT`
- 大写宏：`CURLUE_NO_HOST`
- 大写宏：`CURLUE_OK`
- 大写宏：`CURLU_ALLOW_SPACE`
- 大写宏：`CURLU_DEFAULT_SCHEME`
- 大写宏：`CURLU_GUESS_SCHEME`
- 大写宏：`CURLU_NON_SUPPORT_SCHEME`
- 大写宏：`CURLU_NO_AUTHORITY`
- 大写宏：`CURLU_PATH_AS_IS`
- 大写宏：`CURLU_URLENCODE`
- 大写宏：`HO0_`
- 大写宏：`HTTP`
- 大写宏：`NULL`
- 大写宏：`URL`
- 外部类型：`CURLUcode`
- 外部类型：`Set`
- 外部类型：`That`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-7983968c53` → 本草稿移入 `cases/defect/auto-curl-7983968c53/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
