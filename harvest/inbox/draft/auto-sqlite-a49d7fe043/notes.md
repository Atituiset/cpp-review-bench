# auto-sqlite-a49d7fe043

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b6dd816264a9f345bae061380feb4000f8d72f1c](https://github.com/sqlite/sqlite/commit/b6dd816264a9f345bae061380feb4000f8d72f1c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b6dd816264a9f345bae061380feb4000f8d72f1c (https://github.com/sqlite/sqlite/commit/b6dd816264a9f345bae061380feb4000f8d72f1c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 3234；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b6dd816264a9f345bae061380feb4000f8d72f1c Fix a possible NULL pointer dereference in the (undocumented, untested, :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3202,6 +3202,9 @@ static void percentValue(sqlite3_context *pCtx){
 **
 ** Return JSON text that describes low-level debug/diagnostic information
 ** about the sqlite3_file object associated with SCHEMA.
+**
+** THIS FUNCTION IS UNDOCUMENTED AND UNTESTED.  It might be removed at
+** any moment.  Do not use in production.  Debugging and testing use only.
 */
 static void filestatFunc(
   sqlite3_context *context,
@@ -3231,7 +3234,7 @@ static void filestatFunc(
       rc = sqlite3OsFileControl(fd, SQLITE_FCNTL_FILESTAT, pStr);
       if( rc ) sqlite3_str_append(pStr, "null", 4);
       fd = sqlite3PagerJrnlFile(pPager);
-      if( fd && fd->pMethods!=0 ){
+      if( fd && fd->pMethods!=0 && fd->pMethods->xFileControl!=0 ){
         sqlite3_str_appendall(pStr, ",\"journal\":");
         rc = sqlite3OsFileControl(fd, SQLITE_FCNTL_FILESTAT, pStr);
         if( rc ) sqlite3_str_append(pStr, "null", 4);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3BtreeEnter`
- 外部函数：`sqlite3BtreeLeave`
- 外部函数：`sqlite3BtreePager`
- 外部函数：`sqlite3DbNameToBtree`
- 外部函数：`sqlite3OsFileControl`
- 外部函数：`sqlite3PagerFile`
- 外部函数：`sqlite3PagerJrnlFile`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_result_str`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendall`
- 外部函数：`sqlite3_str_errcode`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_value_text`
- 大写宏：`JSON`
- 大写宏：`SCHEMA`
- 大写宏：`SQLITE_FCNTL_FILESTAT`
- 大写宏：`SQLITE_FINISH`
- 大写宏：`SQLITE_STATIC`
- 外部类型：`Btree`
- 外部类型：`Pager`
- 外部类型：`Return`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a49d7fe043` → 本草稿移入 `cases/defect/auto-sqlite-a49d7fe043/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
