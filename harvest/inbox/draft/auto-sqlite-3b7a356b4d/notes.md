# auto-sqlite-3b7a356b4d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60](https://github.com/sqlite/sqlite/commit/4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 51 |
| 编译错误数（gcc syntax-only） | 61（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60 (https://github.com/sqlite/sqlite/commit/4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2341；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60 Do not synthesize position-lists for UNINDEXED fts5 columns in detail=none mode. Report [bugs:/info/2026-10-20T20:40:52Z :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2338,15 +2338,17 @@ static int fts5CsrPoslist(
         rc = fts5SeekCursor(pCsr, 0);
       }
       for(i=0; i<pConfig->nCol && rc==SQLITE_OK; i++){
-        const char *z = 0;
-        int n = 0; 
-        rc = fts5TextFromStmt(pConfig, pCsr->pStmt, i, &z, &n);
-        if( rc==SQLITE_OK ){
-          rc = sqlite3Fts5ExprPopulatePoslists(
-              pConfig, pCsr->pExpr, aPopulator, i, z, n
-          );
+        if( pConfig->abUnindexed[i]==0 ){
+          const char *z = 0;
+          int n = 0; 
+          rc = fts5TextFromStmt(pConfig, pCsr->pStmt, i, &z, &n);
+          if( rc==SQLITE_OK ){
+            rc = sqlite3Fts5ExprPopulatePoslists(
+                pConfig, pCsr->pExpr, aPopulator, i, z, n
+            );
+          }
+          sqlite3Fts5ClearLocale(pConfig);
         }
-        sqlite3Fts5ClearLocale(pConfig);
       }
       sqlite3_free(aPopulator);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`fts5StmtType`
- 外部函数：`malloc`
- 外部函数：`memcmp`
- 外部函数：`object`
- 外部函数：`sqlite3Fts5ExprPopulatePoslists`
- 外部函数：`sqlite3Fts5ExprRowid`
- 外部函数：`sqlite3Fts5StorageStmt`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_column_value`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`value_blob`
- 外部函数：`value_bytes`
- 外部函数：`zeroblob`
- 大写宏：`FTS5_CONTENT_EXTERNAL`
- 大写宏：`FTS5_CONTENT_NORMAL`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`FTS5_STMT_LOOKUP`
- 大写宏：`FTS5_STMT_SCAN_ASC`
- 大写宏：`FTS5_STMT_SCAN_DESC`
- 大写宏：`MATCH`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`SORTED_MATCH`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_MISMATCH`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 外部类型：`Associated`
- 外部类型：`Call`
- 外部类型：`Default`
- 外部类型：`First`
- 外部类型：`Fts5Auxiliary`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Cursor`
- 外部类型：`Fts5FullTable`
- 外部类型：`Fts5Global`
- 外部类型：`Fts5Table`
- 外部类型：`Fts5TokenizerConfig`
- 外部类型：`Fts5TokenizerModule`
- 外部类型：`If`
- 外部类型：`No`
- 外部类型：`Used`
- 外部类型：`User`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-3b7a356b4d` → 本草稿移入 `cases/defect/auto-sqlite-3b7a356b4d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
