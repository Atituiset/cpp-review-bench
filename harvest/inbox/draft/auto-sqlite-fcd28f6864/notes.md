# auto-sqlite-fcd28f6864

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#a837fbdc69a997ebf43c963bd2838d81d5d3b513](https://github.com/sqlite/sqlite/commit/a837fbdc69a997ebf43c963bd2838d81d5d3b513) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a837fbdc69a997ebf43c963bd2838d81d5d3b513 (https://github.com/sqlite/sqlite/commit/a837fbdc69a997ebf43c963bd2838d81d5d3b513)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a837fbdc69a997ebf43c963bd2838d81d5d3b513 Do not allow incrblob writes to a column that appears in the WHERE clause of :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -116,6 +116,20 @@ static int blobSeekToRow(Incrblob *p, sqlite3_int64 iRow, char **pzErr){
   return rc;
 }
 
+/*
+** Expression-walker callback used by sqlite3_blob_open().  Set
+** pWalker->eCode to 1 and abort the walk if the expression is a reference
+** to column pWalker->u.n of the table.  A WHERE clause on a partial index
+** refers only to columns of the indexed table.
+*/
+static int blobColumnRef(Walker *pWalker, Expr *pExpr){
+  if( pExpr->op==TK_COLUMN && pExpr->iColumn==pWalker->u.n ){
+    pWalker->eCode = 1;
+    return WRC_Abort;
+  }
+  return WRC_Continue;
+}
+
 /*
 ** Open a blob handle.
 */
@@ -238,6 +252,16 @@ int sqlite3_blob_open(
             zFault = "indexed";
           }
         }
+        if( pIdx->pPartIdxWhere ){
+          /* A column read by a partial index WHERE clause determines which
+          ** rows belong in the index, so it cannot be changed either. */
+          Walker w;
+          memset(&w, 0, sizeof(w));
+          w.xExprCallback = blobColumnRef;
+          w.u.n = iCol;
+          sqlite3WalkExpr(&w, pIdx->pPartIdxWhere);
+          if( w.eCode ) zFault = "indexed";
+        }
       }
       if( zFault ){
         sqlite3DbFree(db, zErr);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3DbFree`
- 外部类型：`Open`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-sqlite-fcd28f6864` → 本草稿移入 `cases/defect/auto-sqlite-fcd28f6864/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
