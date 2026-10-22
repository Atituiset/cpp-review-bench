# auto-sqlite-4dc9f0b600

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4259cf526426208b1e5514e5f04c056e68958ebc](https://github.com/sqlite/sqlite/commit/4259cf526426208b1e5514e5f04c056e68958ebc) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4259cf526426208b1e5514e5f04c056e68958ebc (https://github.com/sqlite/sqlite/commit/4259cf526426208b1e5514e5f04c056e68958ebc)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 267；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4259cf526426208b1e5514e5f04c056e68958ebc Avoid orphaning an fts5 %_content table when dropping or renaming an fts5 table that uses contentless_unindexed=1. Repor :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -264,11 +264,15 @@ int sqlite3Fts5DropAll(Fts5Config *pConfig){
         pConfig->zDb, pConfig->zName
     );
   }
-  if( rc==SQLITE_OK && pConfig->eContent==FTS5_CONTENT_NORMAL ){
-    rc = fts5ExecPrintf(pConfig->db, 0, 
-        "DROP TABLE IF EXISTS %Q.'%q_content';",
-        pConfig->zDb, pConfig->zName
-    );
+  if( rc==SQLITE_OK ){
+    if( pConfig->eContent==FTS5_CONTENT_NORMAL 
+     || pConfig->bContentlessUnindexed 
+    ){
+      rc = fts5ExecPrintf(pConfig->db, 0, 
+          "DROP TABLE IF EXISTS %Q.'%q_content';",
+          pConfig->zDb, pConfig->zName
+      );
+    }
   }
   return rc;
 }
@@ -297,7 +301,9 @@ int sqlite3Fts5StorageRename(Fts5Storage *pStorage, const char *zName){
   if( pConfig->bColumnsize ){
     fts5StorageRenameOne(pConfig, &rc, "docsize", zName);
   }
-  if( pConfig->eContent==FTS5_CONTENT_NORMAL ){
+  if( pConfig->eContent==FTS5_CONTENT_NORMAL
+   || pConfig->bContentlessUnindexed 
+  ){
     fts5StorageRenameOne(pConfig, &rc, "content", zName);
   }
   return rc;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`ALTER`
- 大写宏：`DROP`
- 大写宏：`EXISTS`
- 大写宏：`FTS5`
- 大写宏：`FTS5_CONTENT_NORMAL`
- 大写宏：`OUT`
- 大写宏：`RENAME`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`TABLE`
- 外部类型：`Current`
- 外部类型：`Error`
- 外部类型：`Fts5Config`
- 外部类型：`New`
- 外部类型：`Tail`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4dc9f0b600` → 本草稿移入 `cases/defect/auto-sqlite-4dc9f0b600/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
