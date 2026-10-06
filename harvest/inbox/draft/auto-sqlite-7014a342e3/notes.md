# auto-sqlite-7014a342e3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#f0c7055db79a8827dfd487acb80713b8af63a99b](https://github.com/sqlite/sqlite/commit/f0c7055db79a8827dfd487acb80713b8af63a99b) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 106 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f0c7055db79a8827dfd487acb80713b8af63a99b (https://github.com/sqlite/sqlite/commit/f0c7055db79a8827dfd487acb80713b8af63a99b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 1749；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f0c7055db79a8827dfd487acb80713b8af63a99b Omit the -r command-line option from Lemon.  That does not affect :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1746,7 +1746,6 @@ int main(int argc, char **argv){
   static int statistics = 0;
   static int mhflag = 0;
   static int nolinenosflag = 0;
-  static int noResort = 0;
   static int sqlFlag = 0;
   static int printPP = 0;
   
@@ -1764,7 +1763,6 @@ int main(int argc, char **argv){
     {OPT_FLAG, "p", (char*)&showPrecedenceConflict,
                     "Show conflicts resolved by precedence rules"},
     {OPT_FLAG, "q", (char*)&quiet, "(Quiet) Don't print the report file."},
-    {OPT_FLAG, "r", (char*)&noResort, "Do not sort or renumber states"},
     {OPT_FLAG, "s", (char*)&statistics,
                                    "Print parser stats to standard output."},
     {OPT_FLAG, "S", (char*)&sqlFlag,
@@ -1878,7 +1876,7 @@ int main(int argc, char **argv){
     /* Reorder and renumber the states so that states with fewer choices
     ** occur at the end.  This is an optimization that helps make the
     ** generated parser tables smaller. */
-    if( noResort==0 ) ResortStates(&lem);
+    ResortStates(&lem);
 
     /* Generate a report of the parser generated.  (the "y.output" file) */
     if( !quiet ) ReportOutput(&lem);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`exit`
- 外部函数：`fclose`
- 外部函数：`fopen`
- 外部函数：`fprintf`
- 外部函数：`isdigit`
- 外部函数：`lemonStrlen`
- 外部函数：`malloc`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`qsort`
- 外部函数：`strlen`
- 外部函数：`strrchr`
- 外部函数：`symbol`
- 外部函数：`this`
- 外部函数：`va_arg`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`ACCEPT`
- 大写宏：`COMPLETE`
- 大写宏：`ERROR`
- 大写宏：`FILE`
- 大写宏：`INCOMPLETE`
- 大写宏：`LEFT`
- 大写宏：`LEMON_FALSE`
- 大写宏：`LEMON_TRUE`
- 大写宏：`LHS`
- 大写宏：`MULTI`
- 大写宏：`MULTITERMINAL`
- 大写宏：`NONE`
- 大写宏：`NONTERMINAL`
- 大写宏：`NOT_USED`
- 大写宏：`NULL`
- 大写宏：`OPT_FLAG`
- 大写宏：`RD_RESOLVED`
- 大写宏：`REDUCE`
- 大写宏：`RHS`
- 大写宏：`RIGHT`
- 大写宏：`RRCONFLICT`
- 大写宏：`SHIFT`
- 大写宏：`SHIFTREDUCE`
- 大写宏：`SH_RESOLVED`
- 大写宏：`SRCONFLICT`
- 大写宏：`SSCONFLICT`
- 大写宏：`TERMINAL`
- 大写宏：`TERMINALS`
- 大写宏：`UNK`
- 外部类型：`Accept`
- 外部类型：`Alias`
- 外部类型：`All`
- 外部类型：`An`
- 外部类型：`Array`
- 外部类型：`Associativity`
- 外部类型：`Breakdown`
- 外部类型：`Bytes`
- 外部类型：`Can`
- 外部类型：`Code`
- 外部类型：`Command`
- 外部类型：`Declaration`
- 外部类型：`Default`
- 外部类型：`Deleted`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Error`
- 外部类型：`Failed`
- 外部类型：`Field`
- 外部类型：`First`
- 外部类型：`Follow`
- 外部类型：`From`
- 外部类型：`Function`
- 外部类型：`Generate`
- 外部类型：`In`
- 外部类型：`Indent`
- 外部类型：`Index`
- 外部类型：`Init`
- 外部类型：`Left`
- 外部类型：`Line`
- 外部类型：`Linked`
- 外部类型：`List`
- 外部类型：`MULTITERMINALs`
- 外部类型：`Maximum`
- 外部类型：`MemChunk`
- 外部类型：`Minimum`
- 外部类型：`NTs`
- 外部类型：`Name`
- 外部类型：`Negative`
- 外部类型：`Next`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`Only`
- 外部类型：`Out`
- 外部类型：`Parsing`
- 外部类型：`Pointer`
- 外部类型：`Precedence`
- 外部类型：`Print`
- 外部类型：`Quiet`
- 外部类型：`Reduce`
- 外部类型：`Reorder`
- 外部类型：`Rule`
- 外部类型：`Rules`
- 外部类型：`Sequential`
- 外部类型：`Set`
- 外部类型：`Setup`
- 外部类型：`Shift`
- 外部类型：`Show`
- 外部类型：`Since`
- 外部类型：`Size`
- 外部类型：`Slots`
- 外部类型：`Sorted`
- 外部类型：`State`
- 外部类型：`Symbols`
- 外部类型：`Syntax`
- 外部类型：`Table`
- 外部类型：`Text`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Token`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Type`
- 外部类型：`Was`
- 外部类型：`is`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7014a342e3` → 本草稿移入 `cases/defect/auto-sqlite-7014a342e3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
