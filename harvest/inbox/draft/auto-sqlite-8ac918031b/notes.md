# auto-sqlite-8ac918031b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cb6a35b9c2b10b8663958e612acb8a82b04829fd](https://github.com/sqlite/sqlite/commit/cb6a35b9c2b10b8663958e612acb8a82b04829fd) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cb6a35b9c2b10b8663958e612acb8a82b04829fd (https://github.com/sqlite/sqlite/commit/cb6a35b9c2b10b8663958e612acb8a82b04829fd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 425；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cb6a35b9c2b10b8663958e612acb8a82b04829fd Provide an error message when a ROLLBACK changes content out from under :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -422,13 +422,15 @@ static SQLITE_NOINLINE int blobReadWriteRetry(
   }else{
     rc = blobSeekToRow(p, iRow, &zErr);
   }
-  sqlite3DbFree(db, zErr);
   assert( rc==SQLITE_OK || p->pStmt==0 );
   if( rc==SQLITE_OK ){
+    sqlite3DbFree(db, zErr);
     p->bRetry = 1;
     rc = blobReadWrite((sqlite3_blob*)p,z,n,iOffset,xCall);
     p->bRetry = 0;
   }else{
+    sqlite3ErrorWithMsg(db, rc, (zErr ? "%s" : (char*)0), zErr);
+    sqlite3DbFree(db, zErr);
     rc = sqlite3ApiExit(db, rc);
   }
   sqlite3_mutex_leave(db->mutex);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`int`
- 外部函数：`sqlite3ApiExit`
- 外部函数：`sqlite3BtreeCursorIsValidNN`
- 外部函数：`sqlite3BtreeCursorRestore`
- 外部函数：`sqlite3BtreeEnterCursor`
- 外部函数：`sqlite3BtreeIncrblobCursor`
- 外部函数：`sqlite3BtreeIntegerKey`
- 外部函数：`sqlite3BtreeLeaveCursor`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3FaultSim`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3VdbeExec`
- 外部函数：`sqlite3VdbeFinalize`
- 外部函数：`sqlite3VdbeMemSetInt64`
- 外部函数：`sqlite3VdbePreUpdateHook`
- 外部函数：`sqlite3VdbeSerialTypeLen`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_mutex_enter`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`sqlite3_step`
- 外部函数：`testcase`
- 外部函数：`xCall`
- 大写宏：`API`
- 大写宏：`CURSOR_INVALID`
- 大写宏：`CURTYPE_BTREE`
- 大写宏：`NEVER`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_DELETE`
- 大写宏：`SQLITE_DONE`
- 大写宏：`SQLITE_ENABLE_PREUPDATE_HOOK`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_MISUSE_BKPT`
- 大写宏：`SQLITE_MOVED`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_UPDATE`
- 大写宏：`TODO`
- 外部类型：`And`
- 外部类型：`BtCursor`
- 外部类型：`Call`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Incrblob`
- 外部类型：`It`
- 外部类型：`OP_Goto`
- 外部类型：`OP_NotExists`
- 外部类型：`OP_ResultRow`
- 外部类型：`Refresh`
- 外部类型：`Request`
- 外部类型：`Return`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Vdbe`
- 外部类型：`VdbeCursor`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-8ac918031b` → 本草稿移入 `cases/defect/auto-sqlite-8ac918031b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
