# auto-sqlite-6fb8290878

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#a595e9daaa8ec4d4bd238a94cf705103380e9379](https://github.com/sqlite/sqlite/commit/a595e9daaa8ec4d4bd238a94cf705103380e9379) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a595e9daaa8ec4d4bd238a94cf705103380e9379 (https://github.com/sqlite/sqlite/commit/a595e9daaa8ec4d4bd238a94cf705103380e9379)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 448；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a595e9daaa8ec4d4bd238a94cf705103380e9379 Do not allow ALTER TABLE to add a NOT NULL column with a default expression that is not a literal NULL but evaluates to  :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -432,28 +432,12 @@ void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef){
   }
   if( isRowidAlias(pParse, pTab, pCol->zCnName, "add") ) return;
   if( (pCol->colFlags & COLFLAG_GENERATED)==0 ){
-    /* If the default value for the new column was specified with a
-    ** literal NULL, then set pDflt to 0. This simplifies checking
-    ** for an SQL NULL default below.
-    */
     assert( pDflt==0 || pDflt->op==TK_SPAN );
-    if( pDflt && pDflt->pLeft->op==TK_NULL ){
-      pDflt = 0;
-    }
-    assert( IsOrdinaryTable(pNew) );
-    if( (db->flags&SQLITE_ForeignKeys) && pNew->u.tab.pFKey && pDflt ){
-      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
-          "Cannot add a REFERENCES column with non-NULL default value");
-    }
-    if( pCol->notNull && !pDflt ){
-      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
-          "Cannot add a NOT NULL column with default value NULL");
-    }
-
 
-    /* Ensure the default expression is something that sqlite3ValueFromExpr()
-    ** can handle (i.e. not CURRENT_TIME etc.)
-    */
+    /* Ensure the default expression, if any, is something that
+    ** sqlite3ValueFromExpr() can handle (i.e. not CURRENT_TIME etc.).
+    ** If the default expression evaluates to NULL, then set pDflt to 
+    ** 0. This simplifies the checks below.  */
     if( pDflt ){
       sqlite3_value *pVal = 0;
       int rc;
@@ -466,9 +450,22 @@ void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef){
       if( !pVal ){
         sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
            "Cannot add a column with non-constant default");
+      }else if( sqlite3_value_type(pVal)==SQLITE_NULL ){
+        pDflt = 0;
       }
       sqlite3ValueFree(pVal);
     }
+
+    assert( IsOrdinaryTable(pNew) );
+    if( (db->flags&SQLITE_ForeignKeys) && pNew->u.tab.pFKey && pDflt ){
+      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
+          "Cannot add a REFERENCES column with non-NULL default value");
+    }
+    if( pCol->notNull && !pDflt ){
+      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
+          "Cannot add a NOT NULL column with default value NULL");
+    }
+
   }else if( pCol->colFlags & COLFLAG_STORED ){
     sqlite3ErrorIfNotEmpty(pParse, zDb, zTab, "cannot add a STORED column");
   }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`handle`
- 外部函数：`raise`
- 外部函数：`sqlite3ErrorMsg`
- 外部函数：`sqlite3IsRowid`
- 外部函数：`sqlite3NestedParse`
- 外部函数：`sqlite3ValueFree`
- 大写宏：`ABORT`
- 大写宏：`COLFLAG_GENERATED`
- 大写宏：`COLFLAG_STORED`
- 大写宏：`CURRENT_TIME`
- 大写宏：`FROM`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`REFERENCES`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_OK`
- 大写宏：`STORED`
- 大写宏：`TK_NULL`
- 大写宏：`TK_SPAN`
- 外部类型：`Cannot`
- 外部类型：`Ensure`
- 外部类型：`Error`
- 外部类型：`If`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`SQLITE_ForeignKeys`
- 外部类型：`Schema`
- 外部类型：`Table`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-6fb8290878` → 本草稿移入 `cases/defect/auto-sqlite-6fb8290878/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
