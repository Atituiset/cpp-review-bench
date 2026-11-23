# auto-sqlite-c7b2f7ee9b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#fe3fb0a154009420268c5b7d1afd5a860a03b4b7](https://github.com/sqlite/sqlite/commit/fe3fb0a154009420268c5b7d1afd5a860a03b4b7) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 230 |
| 编译错误数（gcc syntax-only） | 330（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fe3fb0a154009420268c5b7d1afd5a860a03b4b7 (https://github.com/sqlite/sqlite/commit/fe3fb0a154009420268c5b7d1afd5a860a03b4b7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 5887；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fe3fb0a154009420268c5b7d1afd5a860a03b4b7 Fix to UPDATE in FTS5 secure-delete when the doclist is large. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5881,12 +5881,19 @@ static void fts5FlushOneHash(Fts5Index *p){
                     continue;
                   }
                 }
-              }else if( (pDoclist[iOff] & 0x01) 
-                && !fts5FlushSecureDelete(p, pStruct, zTerm, nTerm, iRowid)
-              ){
-                if( p->rc!=SQLITE_OK || pDoclist[iOff]==0x01 ){
-                  iOff++;
-                  continue;
+              }else{
+                /* The delete flag is the low bit of the size varint, which
+                ** is not in the first byte if the size field is multi-byte. */
+                int nDummy = 0;
+                int bDel = 0;
+                fts5GetPoslistSize(&pDoclist[iOff], &nDummy, &bDel);
+                if( bDel
+                 && !fts5FlushSecureDelete(p, pStruct, zTerm, nTerm, iRowid)
+                ){
+                  if( p->rc!=SQLITE_OK || pDoclist[iOff]==0x01 ){
+                    iOff++;
+                    continue;
+                  }
                 }
               }
             }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`and`
- 外部函数：`assert`
- 外部函数：`assert_nc`
- 外部函数：`flag`
- 外部函数：`fts5BufferAppendBlob`
- 外部函数：`fts5BufferFree`
- 外部函数：`fts5BufferGrow`
- 外部函数：`fts5BufferSafeAppendBlob`
- 外部函数：`fts5BufferSet`
- 外部函数：`fts5BufferZero`
- 外部函数：`fts5ChunkIterate`
- 外部函数：`fts5DataXXX`
- 外部函数：`fts5DlidxIterFirst`
- 外部函数：`fts5DlidxIterLast`
- 外部函数：`fts5DlidxIterNextR`
- 外部函数：`fts5DlidxIterPrevR`
- 外部函数：`fts5FastGetVarint32`
- 外部函数：`fts5GetU32`
- 外部函数：`fts5GetU64`
- 外部函数：`fts5GetVarint`
- 外部函数：`fts5GetVarint32`
- 外部函数：`fts5IndexColsetTest`
- 外部函数：`fts5IndexCorruptIter`
- 外部函数：`fts5LeafFirstRowidOff`
- 外部函数：`fts5LeafIsTermless`
- 外部函数：`fts5Memcmp`
- 外部函数：`fts5MultiIterDoCompare`
- 外部函数：`fts5PutU16`
- 外部函数：`fts5TrimSegments`
- 外部函数：`fts5_dri`
- 外部函数：`iNextOff`
- 外部函数：`iterate`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`merge`
- 外部函数：`offsetof`
- 外部函数：`on`
- 外部函数：`prepare`
- 外部函数：`sqlite3Fts5BufferSet`
- 外部函数：`sqlite3Fts5BufferSize`
- 外部函数：`sqlite3Fts5ConfigErrmsg`
- 外部函数：`sqlite3Fts5GetVarint`
- 外部函数：`sqlite3Fts5HashQuery`
- 外部函数：`sqlite3Fts5HashScanEntry`
- 外部函数：`sqlite3Fts5HashScanInit`
- 外部函数：`sqlite3Fts5HashScanNext`
- 外部函数：`sqlite3Fts5IndexSync`
- 外部函数：`sqlite3Fts5MallocZero`
- 外部函数：`sqlite3Fts5PutVarint`
- 外部函数：`sqlite3_bind_blob`
- 外部函数：`sqlite3_bind_int`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_blob_bytes`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_blob_reopen`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v3`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`structure`
- 外部函数：`szLeaf`
- 外部函数：`term`
- 外部函数：`the`
- 外部函数：`to`
- 外部函数：`xNext`
- 外部函数：`xSetOutputs`
- 大写宏：`AND`
- 大写宏：`ASC`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`EOF`
- 大写宏：`FROM`
- 大写宏：`FTS`
- 大写宏：`FTS5`
- 大写宏：`FTS5INDEX_QUERY_DESC`
- 大写宏：`FTS5INDEX_QUERY_NOOUTPUT`
- 大写宏：`FTS5INDEX_QUERY_SCAN`
- 大写宏：`FTS5INDEX_QUERY_SCANONETERM`
- 大写宏：`FTS5INDEX_QUERY_SKIPEMPTY`
- 大写宏：`FTS5INDEX_QUERY_SKIPHASH`
- 大写宏：`FTS5INDEX_QUERY_XXX`
- 大写宏：`FTS5INDEX_XXX`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`FTS5_CURRENT_VERSION`
- 大写宏：`FTS5_CURRENT_VERSION_SECUREDELETE`
- 大写宏：`FTS5_DATA_ZERO_PADDING`
- 大写宏：`FTS5_DETAIL_COLUMNS`
- 大写宏：`FTS5_DETAIL_FULL`
- 大写宏：`FTS5_DETAIL_NONE`
- 大写宏：`FTS5_DLIDX_ROWID`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LARGEST_INT64`
- 大写宏：`LIMIT`
- 大写宏：`MIN`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`SECUREDELETE`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SMALLEST_INT64`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_PREPARE_FROM_DDL`
- 大写宏：`SQLITE_PREPARE_NO_VTAB`
- 大写宏：`SQLITE_PREPARE_PERSISTENT`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`SZ_FTS5DLIDXITER`
- 大写宏：`TOMBSTONE_KEYSIZE`
- 大写宏：`TOMBSTONE_NSLOT`
- 大写宏：`UNUSED_PARAM`
- 大写宏：`UNUSED_PARAM2`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Add`
- 外部类型：`Advance`
- 外部类型：`After`
- 外部类型：`All`
- 外部类型：`Allocate`
- 外部类型：`Assuming`
- 外部类型：`At`
- 外部类型：`Back`
- 外部类型：`Backend`
- 外部类型：`Buffer`
- 外部类型：`But`
- 外部类型：`Bytes`
- 外部类型：`Call`
- 外部类型：`Check`
- 外部类型：`Colset`
- 外部类型：`Compare`
- 外部类型：`Copy`
- 外部类型：`Current`
- 外部类型：`Currently`
- 外部类型：`Description`
- 外部类型：`Distance`
- 外部类型：`Doclist`
- 外部类型：`Either`
- 外部类型：`Error`
- 外部类型：`Figure`
- 外部类型：`Footer`
- 外部类型：`Fts5`
- 外部类型：`Fts5Buffer`
- 外部类型：`Fts5CResult`
- 外部类型：`Fts5Colset`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Data`
- 外部类型：`Fts5DlidxIter`
- 外部类型：`Fts5DlidxLvl`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5Iter`
- 外部类型：`Fts5SegIter`
- 外部类型：`Fts5Structure`
- 外部类型：`Fts5StructureLevel`
- 外部类型：`Fts5StructureSegment`
- 外部类型：`Fts5TombstoneArray`
- 外部类型：`Hash`
- 外部类型：`Id`
- 外部类型：`If`
- 外部类型：`Immediately`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`Initial`
- 外部类型：`Initialize`
- 外部类型：`Input`
- 外部类型：`Iterator`
- 外部类型：`Leaf`
- 外部类型：`Leave`
- 外部类型：`Left`
- 外部类型：`Level`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Mask`
- 外部类型：`Max`
- 外部类型：`Minimum`
- 外部类型：`Modify`
- 外部类型：`Name`
- 外部类型：`New`
- 外部类型：`Next`
- 外部类型：`Number`
- 外部类型：`Object`
- 外部类型：`Of`
- 外部类型：`Offset`
- 外部类型：`One`
- 外部类型：`Or`
- 外部类型：`Page`
- 外部类型：`Populate`
- 外部类型：`Position`
- 外部类型：`PoslistCallbackCtx`
- 外部类型：`PoslistOffsetsCtx`
- 外部类型：`Power`
- 外部类型：`Read`
- 外部类型：`Removing`
- 外部类型：`Replace`
- 外部类型：`Right`
- 外部类型：`Rowid`
- 外部类型：`Search`
- 外部类型：`See`
- 外部类型：`Segment`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Start`
- 外部类型：`State`
- 外部类型：`Structure`
- 外部类型：`Term`
- 外部类型：`Terms`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Unless`
- 外部类型：`Unused`
- 外部类型：`Used`
- 外部类型：`Variables`
- 外部类型：`Virtual`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c7b2f7ee9b` → 本草稿移入 `cases/defect/auto-sqlite-c7b2f7ee9b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
