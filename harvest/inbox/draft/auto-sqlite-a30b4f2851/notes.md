# auto-sqlite-a30b4f2851

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#77b2a47a202f83d5a48aa259f62346ce497af122](https://github.com/sqlite/sqlite/commit/77b2a47a202f83d5a48aa259f62346ce497af122) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #77b2a47a202f83d5a48aa259f62346ce497af122 (https://github.com/sqlite/sqlite/commit/77b2a47a202f83d5a48aa259f62346ce497af122)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 12（原始 PR diff 行 106；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 77b2a47a202f83d5a48aa259f62346ce497af122 Fix compiler warnings, mostly associated with the new ext/misc/optname.c :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -87,7 +87,7 @@ static const struct OptName {
 **     SQLITE_MISUSE       Bad arguments
 */
 int sqlite3_optimization_mask(const char *zName, sqlite3_uint64 *pMask){
-  int i;
+  unsigned int i;
   if( zName==0 || pMask==0 ){ return SQLITE_MISUSE; }
   for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
     if( sqlite3_stricmp(zName, aOpt[i].zName)==0 ){
@@ -103,8 +103,8 @@ int sqlite3_optimization_mask(const char *zName, sqlite3_uint64 *pMask){
 ** Given a bitmap, return the canonical symbolic name.  Return 0 for
 ** a bitmap input of 0 or a bitmap value that matches nothing.
 */
-const char *sqlite3_optimization_name(sqlite3_int64 mask){
-  int i;
+const char *sqlite3_optimization_name(sqlite3_uint64 mask){
+  unsigned int i;
   for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
     if( aOpt[i].uMask==mask ){ return aOpt[i].zName; }
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_stricmp`
- 大写宏：`SQLITE_MISUSE`
- 外部类型：`Bad`
- 外部类型：`Given`
- 外部类型：`Return`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a30b4f2851` → 本草稿移入 `cases/defect/auto-sqlite-a30b4f2851/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
