# auto-sqlite-d3487acb15

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#bf181220a009b89d782cf6bf9a933683255f7ead](https://github.com/sqlite/sqlite/commit/bf181220a009b89d782cf6bf9a933683255f7ead) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bf181220a009b89d782cf6bf9a933683255f7ead (https://github.com/sqlite/sqlite/commit/bf181220a009b89d782cf6bf9a933683255f7ead)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1332；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bf181220a009b89d782cf6bf9a933683255f7ead Still more hardening against smuggled shell command attacks. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1329,9 +1329,19 @@ static int recoverWriteSchema2(sqlite3_recover *p){
   if( pSelect ){
     while( sqlite3_step(pSelect)==SQLITE_ROW ){
       const char *zSql = (const char*)sqlite3_column_text(pSelect, 1);
-      int rc = recoverOneStmt(p->dbOut, zSql, 0);
+      const char *zTail = 0;
+      int rc = recoverOneStmt(p->dbOut, zSql, &zTail);
       if( rc==SQLITE_OK ){
+        char *pToFree = 0;
+        if( zTail && zTail[0] ){
+          /* Omit any extraneous text that follows the first SQL statement */
+          int n = (int)(zTail - zSql);
+          while( n>0 && (zSql[n-1]==';' || zSql[n-1]<=' ') ){ n--; }
+          pToFree = sqlite3_mprintf("%.*s", n, zSql);
+          zSql = pToFree;
+        }
         recoverSqlCallback(p, zSql);
+        sqlite3_free(pToFree);
       }else if( rc!=SQLITE_ERROR ){
         recoverDbError(p, p->dbOut);
       }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`db`
- 外部函数：`int`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_recover_config`
- 外部函数：`sqlite3_recover_errcode`
- 外部函数：`sqlite3_recover_errmsg`
- 外部函数：`sqlite3_recover_run`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`table`
- 外部函数：`use`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xSql`
- 大写宏：`NULL`
- 大写宏：`RECOVER_STATE_LOSTANDFOUND`
- 大写宏：`RECOVER_STATE_WRITING`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RECOVER_FREELIST_CORRUPT`
- 大写宏：`SQLITE_RECOVER_ROWIDS`
- 大写宏：`SQLITE_RECOVER_SLOWINDEXES`
- 大写宏：`SQLITE_ROW`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-d3487acb15` → 本草稿移入 `cases/defect/auto-sqlite-d3487acb15/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
