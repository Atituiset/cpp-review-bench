# auto-sqlite-2b18178bd1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#3e9779900906ef1fc36a5cdfe9969b7be30ecb14](https://github.com/sqlite/sqlite/commit/3e9779900906ef1fc36a5cdfe9969b7be30ecb14) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3e9779900906ef1fc36a5cdfe9969b7be30ecb14 (https://github.com/sqlite/sqlite/commit/3e9779900906ef1fc36a5cdfe9969b7be30ecb14)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3e9779900906ef1fc36a5cdfe9969b7be30ecb14 Change the sqlite3_serialize() interface to return a NULL pointer if :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -745,7 +745,8 @@ static MemFile *memdbFromDbSchema(sqlite3 *db, const char *zSchema){
 }
 
 /*
-** Return the serialization of a database
+** Return the serialization of a database.  Or return NULL if there
+** is an OOM or I/O error.
 */
 unsigned char *sqlite3_serialize(
   sqlite3 *db,              /* The database connection */
@@ -821,8 +822,12 @@ unsigned char *sqlite3_serialize(
           rc = sqlite3PagerGet(pPager, pgno, (DbPage**)&pPage, 0);
           if( rc==SQLITE_OK ){
             memcpy(pTo, sqlite3PagerGetData(pPage), szPage);
-          }else{
+          }else if( pgno==PENDING_BYTE/szPage+1 ){
             memset(pTo, 0, szPage);
+          }else{
+            sqlite3_free(pOut);
+            pOut = 0;
+            nPage = 0;            
           }
           sqlite3PagerUnref(pPage);       
         }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sqlite3BtreeGetPageSize`
- 外部函数：`sqlite3BtreePager`
- 外部函数：`sqlite3FindDbName`
- 外部函数：`sqlite3PagerGet`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerUnref`
- 外部函数：`sqlite3SafetyCheckOk`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_file_control`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_mutex_enter`
- 外部函数：`sqlite3_mutex_leave`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 大写宏：`BEGIN`
- 大写宏：`COMMIT`
- 大写宏：`IMMEDIATE`
- 大写宏：`NULL`
- 大写宏：`PRAGMA`
- 大写宏：`SQLITE_ENABLE_API_ARMOR`
- 大写宏：`SQLITE_FCNTL_FILE_POINTER`
- 大写宏：`SQLITE_MISUSE_BKPT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_SERIALIZE_NOCOPY`
- 大写宏：`UNUSED_PARAMETER`
- 外部类型：`Btree`
- 外部类型：`DbPage`
- 外部类型：`Maybe`
- 外部类型：`MemFile`
- 外部类型：`MemStore`
- 外部类型：`Pager`
- 外部类型：`Return`
- 外部类型：`The`
- 外部类型：`Which`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-2b18178bd1` → 本草稿移入 `cases/defect/auto-sqlite-2b18178bd1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
