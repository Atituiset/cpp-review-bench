# auto-sqlite-61c837a521

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#5d1ee888094efc63d1fb25a58af93478b103d441](https://github.com/sqlite/sqlite/commit/5d1ee888094efc63d1fb25a58af93478b103d441) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 150 |
| 编译错误数（gcc syntax-only） | 63（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5d1ee888094efc63d1fb25a58af93478b103d441 (https://github.com/sqlite/sqlite/commit/5d1ee888094efc63d1fb25a58af93478b103d441)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 5212；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5d1ee888094efc63d1fb25a58af93478b103d441 Avoid leaving a transaction open in the session module when a changeset applied in no-savepoint mode fails because an UP :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5205,19 +5205,22 @@ static int sessionApplyOneWithRetry(
       assert( pIter->op==SQLITE_INSERT );
       rc = sqlite3_exec(db, "SAVEPOINT replace_op", 0, 0, 0);
       if( rc==SQLITE_OK ){
+        int rc2;
         rc = sessionBindRow(pIter, 
             sqlite3changeset_new, pApply->nCol, pApply->abPK, pApply->pDelete);
         sqlite3_bind_int(pApply->pDelete, pApply->nCol+1, 1);
-      }
-      if( rc==SQLITE_OK ){
-        sqlite3_step(pApply->pDelete);
-        rc = sqlite3_reset(pApply->pDelete);
-      }
-      if( rc==SQLITE_OK ){
-        rc = sessionApplyOneOp(pIter, pApply, xConflict, pCtx, 0, 0);
-      }
-      if( rc==SQLITE_OK ){
-        rc = sqlite3_exec(db, "RELEASE replace_op", 0, 0, 0);
+        if( rc==SQLITE_OK ){
+          sqlite3_step(pApply->pDelete);
+          rc = sqlite3_reset(pApply->pDelete);
+        }
+        if( rc==SQLITE_OK ){
+          rc = sessionApplyOneOp(pIter, pApply, xConflict, pCtx, 0, 0);
+        }
+        if( rc!=SQLITE_OK ){
+          sqlite3_exec(db, "ROLLBACK TO replace_op", 0, 0, 0);
+        }
+        rc2 = sqlite3_exec(db, "RELEASE replace_op", 0, 0, 0);
+        if( rc==SQLITE_OK ) rc = rc2;
       }
     }
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`putVarint32`
- 外部函数：`sessionAppendIdent`
- 外部函数：`sessionPutDouble`
- 外部函数：`sessionPutI64`
- 外部函数：`sessionVarintLen`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3_snprintf`
- 外部函数：`sqlite3_value_blob`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_double`
- 外部函数：`sqlite3_value_int64`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`type`
- 大写宏：`ARM7`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`TEXT`
- 大写宏：`TODO`
- 大写宏：`UPDATE`
- 外部类型：`Allocations`
- 外部类型：`Buffer`
- 外部类型：`Current`
- 外部类型：`Error`
- 外部类型：`If`
- 外部类型：`Increment`
- 外部类型：`Input`
- 外部类型：`It`
- 外部类型：`Iterator`
- 外部类型：`Number`
- 外部类型：`Points`
- 外部类型：`Primary`
- 外部类型：`SQLite`
- 外部类型：`SessionBuffer`
- 外部类型：`SessionInput`
- 外部类型：`Size`
- 外部类型：`Skip`
- 外部类型：`String`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Value`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-61c837a521` → 本草稿移入 `cases/defect/auto-sqlite-61c837a521/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
