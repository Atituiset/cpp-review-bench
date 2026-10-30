# auto-sqlite-82d52bf605

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#05f5ad235f1421a593fb4605e859aefd4c494289](https://github.com/sqlite/sqlite/commit/05f5ad235f1421a593fb4605e859aefd4c494289) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 72 |
| 编译错误数（gcc syntax-only） | 50（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #05f5ad235f1421a593fb4605e859aefd4c494289 (https://github.com/sqlite/sqlite/commit/05f5ad235f1421a593fb4605e859aefd4c494289)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1040；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 05f5ad235f1421a593fb4605e859aefd4c494289 In the Expert extension, correctly quote names that begin with a digit. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -823,6 +823,8 @@ static int idxIdentifierRequiresQuotes(const char *zId){
   int i;
   int nId = STRLEN(zId);
   
+  if( nId==0 ) return 1;
+  if( zId[0]>='0' && zId[0]<='9' ) return 1;
   if( sqlite3_keyword_check(zId, nId) ) return 1;
 
   for(i=0; zId[i]; i++){
@@ -1037,9 +1039,7 @@ static int idxCreateFromCons(
           rc = SQLITE_NOMEM;
         }else{
           rc = sqlite3_exec(dbm, zIdx, 0, 0, p->pzErrmsg);
-          if( rc!=SQLITE_OK ){
-            rc = SQLITE_BUSY_TIMEOUT;
-          }else{
+          if( rc==SQLITE_OK ){
             idxHashAdd(&rc, &p->hIdx, zName, zIdx);
           }
         }
@@ -2160,11 +2160,15 @@ int sqlite3_expert_analyze(sqlite3expert *p, char **pzErr){
 
   /* Create candidate indexes within the in-memory database file */
   if( rc==SQLITE_OK ){
+    p->pzErrmsg = pzErr;
     rc = idxCreateCandidates(p);
-  }else if ( rc==SQLITE_BUSY_TIMEOUT ){
-    if( pzErr )
-      *pzErr = sqlite3_mprintf("Cannot find a unique index name to propose.");
-    return rc;
+    p->pzErrmsg = 0;
+    if( rc==SQLITE_BUSY_TIMEOUT ){
+      if( pzErr ){
+        *pzErr = sqlite3_mprintf("Cannot find a unique index name to propose.");
+      }
+      return rc;
+    }
   }
 
   /* Generate the stat1 data */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`count`
- 外部函数：`idxPrepareStmt`
- 外部函数：`in`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`printf`
- 外部函数：`s`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_keyword_check`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_randomness`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`AND`
- 大写宏：`COLLATE`
- 大写宏：`CREATE`
- 大写宏：`DESC`
- 大写宏：`EXPERT_REPORT_CANDIDATES`
- 大写宏：`FROM`
- 大写宏：`INDEX`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`PRAGMA`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_BUSY_TIMEOUT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`WHERE`
- 外部类型：`After`
- 外部类型：`And`
- 外部类型：`Cannot`
- 外部类型：`Column`
- 外部类型：`Compiled`
- 外部类型：`Count`
- 外部类型：`Create`
- 外部类型：`Database`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`Gather`
- 外部类型：`Gave`
- 外部类型：`Generate`
- 外部类型：`Hash`
- 外部类型：`IdxColumn`
- 外部类型：`IdxConstraint`
- 外部类型：`IdxHash`
- 外部类型：`IdxHashEntry`
- 外部类型：`IdxScan`
- 外部类型：`IdxStatement`
- 外部类型：`IdxTable`
- 外部类型：`IdxWrite`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`Is`
- 外部类型：`List`
- 外部类型：`Number`
- 外部类型：`Percentage`
- 外部类型：`Scan`
- 外部类型：`Suppress`
- 外部类型：`Table`
- 外部类型：`This`
- 外部类型：`Trailing`
- 外部类型：`True`
- 外部类型：`User`
- 外部类型：`Vtab`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-82d52bf605` → 本草稿移入 `cases/defect/auto-sqlite-82d52bf605/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
