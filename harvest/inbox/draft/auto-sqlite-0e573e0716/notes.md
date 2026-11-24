# auto-sqlite-0e573e0716

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#95635a25d5a058dac504649debd970ba2f465801](https://github.com/sqlite/sqlite/commit/95635a25d5a058dac504649debd970ba2f465801) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 115 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #95635a25d5a058dac504649debd970ba2f465801 (https://github.com/sqlite/sqlite/commit/95635a25d5a058dac504649debd970ba2f465801)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 95635a25d5a058dac504649debd970ba2f465801 New OP_NoWriter opcode blocks some DDL statements if there are concurrent :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5227,6 +5227,7 @@ void sqlite3ClearOnOrUsing(sqlite3*, OnOrUsing*);
 void sqlite3SrcListDelete(sqlite3*, SrcList*);
 Index *sqlite3AllocateIndexObject(sqlite3*,int,int,char**);
 int sqlite3IndexBloomable(const Index*,int);
+void sqlite3FailIfConcurrentWriter(Parse*,Table*);
 void sqlite3CreateIndex(Parse*,Token*,Token*,SrcList*,ExprList*,int,Token*,
                           Expr*, int, int, u8);
 void sqlite3DropIndex(Parse*, SrcList*, int);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`code`
- 外部函数：`insert`
- 外部函数：`int`
- 外部函数：`s3_vtab_on_conflict`
- 外部函数：`sqlite3_changes`
- 外部函数：`sqlite3_total_changes`
- 外部函数：`void`
- 外部函数：`xCommitCallback`
- 外部函数：`xOpen`
- 外部函数：`xRollbackCallback`
- 大写宏：`ADD`
- 大写宏：`ATTACH`
- 大写宏：`BINARY`
- 大写宏：`COLUMN`
- 大写宏：`ROWID`
- 大写宏：`SQL`
- 大写宏：`SQLITE_`
- 大写宏：`SQLITE_LIMIT_CTES`
- 大写宏：`SQLITE_OMIT_DEPRECATED`
- 大写宏：`SQLITE_TRACE`
- 大写宏：`SQLITE_TRACE_LEGACY`
- 大写宏：`TEMP`
- 大写宏：`TRUE`
- 大写宏：`VACUUM`
- 外部类型：`All`
- 外部类型：`Argument`
- 外部类型：`Array`
- 外部类型：`Autovac`
- 外部类型：`Building`
- 外部类型：`BusyHandler`
- 外部类型：`Byte`
- 外部类型：`Client`
- 外部类型：`CollSeq`
- 外部类型：`Connection`
- 外部类型：`Current`
- 外部类型：`Db`
- 外部类型：`DbClientData`
- 外部类型：`Default`
- 外部类型：`Do`
- 外部类型：`Errno`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`Flags`
- 外部类型：`Index`
- 外部类型：`Information`
- 外部类型：`Interface`
- 外部类型：`Invoked`
- 外部类型：`Last`
- 外部类型：`Limits`
- 外部类型：`List`
- 外部类型：`Lookaside`
- 外部类型：`Maximum`
- 外部类型：`MemDB`
- 外部类型：`Most`
- 外部类型：`Number`
- 外部类型：`OOMs`
- 外部类型：`OP_SqlExec`
- 外部类型：`OP_VDestroy`
- 外部类型：`Pagesize`
- 外部类型：`Parse`
- 外部类型：`Pgno`
- 外部类型：`PreUpdate`
- 外部类型：`Profiling`
- 外部类型：`Rootpage`
- 外部类型：`Savepoint`
- 外部类型：`See`
- 外部类型：`Significant`
- 外部类型：`SrcList`
- 外部类型：`Text`
- 外部类型：`The`
- 外部类型：`Token`
- 外部类型：`True`
- 外部类型：`VDBEs`
- 外部类型：`VTable`
- 外部类型：`Value`
- 外部类型：`Vdbe`
- 外部类型：`VtabCtx`
- 外部类型：`Which`
- 外部类型：`uint32_t`
- 外部类型：`uint8_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-0e573e0716` → 本草稿移入 `cases/defect/auto-sqlite-0e573e0716/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
