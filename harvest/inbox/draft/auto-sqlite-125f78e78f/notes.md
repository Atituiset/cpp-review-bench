# auto-sqlite-125f78e78f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6de9972df6d8bca1e642feadb438b066375ef072](https://github.com/sqlite/sqlite/commit/6de9972df6d8bca1e642feadb438b066375ef072) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 33（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6de9972df6d8bca1e642feadb438b066375ef072 (https://github.com/sqlite/sqlite/commit/6de9972df6d8bca1e642feadb438b066375ef072)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6de9972df6d8bca1e642feadb438b066375ef072 Fix an fts5 problem causing auxiliary functions to see an incorrect list of matches in some cases when the query contain :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -940,6 +940,13 @@ static int fts5ExprNearTest(
     if( i==pNear->nPhrase && (i==1 || fts5ExprNearIsMatch(pRc, pNear)) ){
       return 1;
     }
+
+    /* Not a match. Clear the position lists of all phrases, as some may
+    ** have been populated before the loop above exited early, and others
+    ** may still contain data left over from a previous row.  */
+    for(i=0; i<pNear->nPhrase; i++){
+      pNear->apPhrase[i]->poslist.n = 0;
+    }
     return 0;
   }
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`int`
- 外部函数：`memset`
- 外部函数：`phrase`
- 外部函数：`search`
- 外部函数：`sqlite3Fts5MallocZero`
- 外部函数：`sqlite3Fts5PoslistNext64`
- 外部函数：`sqlite3Fts5PoslistSafeAppend`
- 外部函数：`sqlite3_free`
- 大写宏：`AND`
- 大写宏：`EOF`
- 大写宏：`FLEXARRAY`
- 大写宏：`FTS5_STRING`
- 大写宏：`NEAR`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`SQLITE_OK`
- 外部类型：`Add`
- 外部类型：`After`
- 外部类型：`Array`
- 外部类型：`Buffer`
- 外部类型：`Child`
- 外部类型：`Columns`
- 外部类型：`Current`
- 外部类型：`Distance`
- 外部类型：`For`
- 外部类型：`Fts5Buffer`
- 外部类型：`Fts5Colset`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Expr`
- 外部类型：`Fts5ExprNearset`
- 外部类型：`Fts5ExprNode`
- 外部类型：`Fts5ExprPhrase`
- 外部类型：`Fts5ExprTerm`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5LookaheadReader`
- 外部类型：`Fts5NearTrimmer`
- 外部类型：`Fts5PoslistWriter`
- 外部类型：`If`
- 外部类型：`Initialize`
- 外部类型：`Iterate`
- 外部类型：`Iterator`
- 外部类型：`Next`
- 外部类型：`Node`
- 外部类型：`Number`
- 外部类型：`Pointers`
- 外部类型：`Terms`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-125f78e78f` → 本草稿移入 `cases/defect/auto-sqlite-125f78e78f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
