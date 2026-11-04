# auto-sqlite-06be2f2734

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a](https://github.com/sqlite/sqlite/commit/936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 69 |
| 编译错误数（gcc syntax-only） | 55（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a (https://github.com/sqlite/sqlite/commit/936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a Fix corruption of a file-backed temporary database caused by committing a transaction that modifies zero pages when the  :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2026,11 +2026,18 @@ static int pager_truncate(Pager *pPager, Pgno nPage);
 **      file has been created already (via a spill on pagerStress()) and
 **      when the number of dirty pages in memory exceeds 25% of the total
 **      cache size.
+**
+**   *  Never sync a TEMP database if the current transaction has not
+**      modified the database (eState<PAGER_WRITER_CACHEMOD). In this case
+**      sqlite3PagerCommitPhaseOne() returns early without flushing any
+**      dirty pages left over from earlier transactions, so they must not
+**      be marked as clean by pager_end_transaction().
 */
 static int pagerFlushOnCommit(Pager *pPager, int bCommit){
   if( pPager->tempFile==0 ) return 1;
   if( !bCommit ) return 0;
   if( !isOpen(pPager->fd) ) return 0;
+  if( pPager->eState<PAGER_WRITER_CACHEMOD ) return 0;
   return (sqlite3PCachePercentDirty(pPager->pPCache)>=25);
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`change`
- 外部函数：`headers`
- 外部函数：`int`
- 外部函数：`isOpen`
- 外部函数：`lock`
- 外部函数：`pagerFlushOnCommit`
- 外部函数：`pagerStress`
- 外部函数：`sqlite3PCachePercentDirty`
- 外部函数：`state`
- 外部函数：`void`
- 外部函数：`xFetch`
- 外部函数：`xOpen`
- 大写宏：`EXCLUSIVE`
- 大写宏：`FCNTL_SIZE_HINT`
- 大写宏：`OPEN`
- 大写宏：`PAGER_JOURNALMODE_`
- 大写宏：`READER`
- 大写宏：`SQLITE_ENABLE_SETLK_TIMEOUT`
- 大写宏：`SQLITE_OMIT_WAL`
- 大写宏：`SQLITE_TEST`
- 大写宏：`SYNC_FULL`
- 大写宏：`SYNC_NORMAL`
- 大写宏：`VFS`
- 大写宏：`WAL`
- 大写宏：`WRITER_LOCKED`
- 外部类型：`Add`
- 外部类型：`Array`
- 外部类型：`Assumed`
- 外部类型：`Bitvec`
- 外部类型：`Boolean`
- 外部类型：`Byte`
- 外部类型：`Call`
- 外部类型：`Changes`
- 外部类型：`Class`
- 外部类型：`Context`
- 外部类型：`Current`
- 外部类型：`Database`
- 外部类型：`DbPage`
- 外部类型：`Desired`
- 外部类型：`Do`
- 外部类型：`End`
- 外部类型：`File`
- 外部类型：`Flags`
- 外部类型：`From`
- 外部类型：`Function`
- 外部类型：`List`
- 外部类型：`Maximum`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`One`
- 外部类型：`PCache`
- 外部类型：`Page`
- 外部类型：`Pager`
- 外部类型：`PagerSavepoint`
- 外部类型：`Pages`
- 外部类型：`PgHdr`
- 外部类型：`Pgno`
- 外部类型：`Pointer`
- 外部类型：`Quasi`
- 外部类型：`Routine`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`Super`
- 外部类型：`The`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Use`
- 外部类型：`Value`
- 外部类型：`Wal`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-06be2f2734` → 本草稿移入 `cases/defect/auto-sqlite-06be2f2734/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
