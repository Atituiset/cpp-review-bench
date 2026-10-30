# auto-sqlite-f1373218f7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cc6c325e99ebabe0e4a5779ade5d28ed5c8aaa98](https://github.com/sqlite/sqlite/commit/cc6c325e99ebabe0e4a5779ade5d28ed5c8aaa98) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cc6c325e99ebabe0e4a5779ade5d28ed5c8aaa98 (https://github.com/sqlite/sqlite/commit/cc6c325e99ebabe0e4a5779ade5d28ed5c8aaa98)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cc6c325e99ebabe0e4a5779ade5d28ed5c8aaa98 Fix the recover extension so that it understands that  :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1167,6 +1167,18 @@ static void recoverAddTable(
     }
     recoverFinalize(p, pStmt);
 
+    /* A column declared as "INTEGER PRIMARY KEY DESC" is not an alias for
+    ** the rowid. Such a table has an automatic index with origin 'pk',
+    ** whereas a table with a true rowid alias does not.  */
+    if( iPk>=0 ){
+      pStmt = recoverPreparePrintf(p, p->dbOut,
+          "SELECT 1 FROM pragma_index_list(%Q, %Q) WHERE origin='pk'",
+          zName, p->zDb
+      );
+      if( pStmt && sqlite3_step(pStmt)==SQLITE_ROW ) iPk = -1;
+      recoverFinalize(p, pStmt);
+    }
+
     if( p->errCode==SQLITE_OK ){
       if( iPk>=0 ){
         pNew->aCol[iPk].bIPK = 1;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`db`
- 外部函数：`int`
- 外部函数：`sqlite3_db_handle`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_recover_config`
- 外部函数：`sqlite3_recover_errcode`
- 外部函数：`sqlite3_recover_errmsg`
- 外部函数：`sqlite3_recover_run`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`table`
- 外部函数：`use`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`NULL`
- 大写宏：`RECOVER_STATE_LOSTANDFOUND`
- 大写宏：`RECOVER_STATE_WRITING`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RECOVER_FREELIST_CORRUPT`
- 大写宏：`SQLITE_RECOVER_ROWIDS`
- 大写宏：`SQLITE_RECOVER_SLOWINDEXES`
- 大写宏：`URI`
- 外部类型：`Copies`
- 外部类型：`Error`
- 外部类型：`Fields`
- 外部类型：`For`
- 外部类型：`Input`
- 外部类型：`List`
- 外部类型：`Name`
- 外部类型：`Output`
- 外部类型：`Pointer`
- 外部类型：`RecoverStateLAF`
- 外部类型：`RecoverStateW1`
- 外部类型：`RecoverTable`
- 外部类型：`State`
- 外部类型：`Values`
- 外部类型：`Variables`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-f1373218f7` → 本草稿移入 `cases/defect/auto-sqlite-f1373218f7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
