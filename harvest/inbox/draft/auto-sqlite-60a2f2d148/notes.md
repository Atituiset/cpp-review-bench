# auto-sqlite-60a2f2d148

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#918766b06c45040cd0bbb61d82145353cc86ef47](https://github.com/sqlite/sqlite/commit/918766b06c45040cd0bbb61d82145353cc86ef47) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 283 |
| 编译错误数（gcc syntax-only） | 250（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #918766b06c45040cd0bbb61d82145353cc86ef47 (https://github.com/sqlite/sqlite/commit/918766b06c45040cd0bbb61d82145353cc86ef47)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 918766b06c45040cd0bbb61d82145353cc86ef47 Prevent a flush-loop in secure-delete of FTS5 which could overflow the :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -366,6 +366,7 @@ struct Fts5Index {
   /* Error state. */
   int rc;                         /* Current error code */
   int flushRc;
+  int bInSync;                    /* True while inside sqlite3Fts5IndexSync() */
 
   /* State used by the fts5DataXXX() functions. */
   sqlite3_blob *pReader;          /* RO incr-blob open on %_data table */
@@ -7011,8 +7012,17 @@ int sqlite3Fts5IndexBeginWrite(Fts5Index *p, int bDelete, i64 iRowid){
 */
 int sqlite3Fts5IndexSync(Fts5Index *p){
   assert( p->rc==SQLITE_OK );
+  /* Flushing may run internal SQL statements (e.g. secure-delete updates of
+  ** the %_idx table). If such a statement opens a statement journal it
+  ** invokes the xSavepoint() method of this same table, which lands back
+  ** here before the outer flush has cleared the hash table. Without this
+  ** guard that recurses until the stack overflows. The outer flush is still
+  ** running and will write everything out, so the nested call is a no-op. */
+  if( p->bInSync ) return SQLITE_OK;
+  p->bInSync = 1;
   fts5IndexFlush(p);
   fts5IndexCloseReader(p);
+  p->bInSync = 0;
   return fts5IndexReturn(p);
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`fts5DataXXX`
- 外部函数：`fts5IndexCorruptIdx`
- 外部函数：`fts5PutU16`
- 外部函数：`fts5_dri`
- 外部函数：`memset`
- 外部函数：`sqlite3_blob_bytes`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_blob_reopen`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`structure`
- 大写宏：`AND`
- 大写宏：`DELETE`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`FROM`
- 大写宏：`FTS5_CORRUPT_ROWID`
- 大写宏：`FTS5_DATA_DLI_B`
- 大写宏：`FTS5_DATA_HEIGHT_B`
- 大写宏：`FTS5_DATA_PAGE_B`
- 大写宏：`INSERT`
- 大写宏：`NULL`
- 大写宏：`SET`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`All`
- 外部类型：`Current`
- 外部类型：`Error`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Data`
- 外部类型：`Fts5DlidxWriter`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5Iter`
- 外部类型：`Fts5PageWriter`
- 外部类型：`Fts5SegIter`
- 外部类型：`Fts5SegWriter`
- 外部类型：`Fts5Structure`
- 外部类型：`Fts5StructureLevel`
- 外部类型：`Fts5StructureSegment`
- 外部类型：`Hash`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Leaf`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Read`
- 外部类型：`Rowid`
- 外部类型：`State`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`Variables`
- 外部类型：`Virtual`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-60a2f2d148` → 本草稿移入 `cases/defect/auto-sqlite-60a2f2d148/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
