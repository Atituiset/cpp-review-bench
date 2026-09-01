# auto-sqlite-63efcf7efb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#99b322eca9244db422f00a0c414185740cf6077f](https://github.com/sqlite/sqlite/commit/99b322eca9244db422f00a0c414185740cf6077f) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 55 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #99b322eca9244db422f00a0c414185740cf6077f (https://github.com/sqlite/sqlite/commit/99b322eca9244db422f00a0c414185740cf6077f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 99b322eca9244db422f00a0c414185740cf6077f Fix an SQLITE_CORRUPT error that could occur with an incremental vacuum database if many tables were created and dropped :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10428,6 +10428,9 @@ static int btreeDropTable(Btree *p, Pgno iTable, int *piMoved){
       }
       pMove = 0;
       rc = btreeGetPage(pBt, maxRootPgno, &pMove, 0);
+      if( rc==SQLITE_OK ){
+        rc = sqlite3PagerWrite(pMove->pDbPage);
+      }
       freePage(pMove, &rc);
       releasePage(pMove);
       if( rc!=SQLITE_OK ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`btreePageFromDbPage`
- 外部函数：`fflush`
- 外部函数：`future`
- 外部函数：`get4byte`
- 外部函数：`memset`
- 外部函数：`put4byte`
- 外部函数：`sqlite3BitvecCreate`
- 外部函数：`sqlite3BitvecSet`
- 外部函数：`sqlite3BitvecSize`
- 外部函数：`sqlite3PagerGet`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerGetExtra`
- 外部函数：`sqlite3PagerLookup`
- 外部函数：`sqlite3PagerRef`
- 外部函数：`sqlite3PagerUnrefNotNull`
- 外部函数：`sqlite3PagerWrite`
- 外部函数：`sqlite3_mutex_held`
- 大写宏：`BTS_SECURE_DELETE`
- 大写宏：`CORRUPT_DB`
- 大写宏：`ISAUTOVACUUM`
- 大写宏：`NULL`
- 大写宏：`PAGER_GET_NOCONTENT`
- 大写宏：`PAGER_GET_READONLY`
- 大写宏：`PTRMAP_FREEPAGE`
- 大写宏：`SQLITE_CORRUPT_BKPT`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 外部类型：`At`
- 外部类型：`BtShared`
- 外部类型：`But`
- 外部类型：`Code`
- 外部类型：`DbPage`
- 外部类型：`Free`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Increment`
- 外部类型：`Initial`
- 外部类型：`Local`
- 外部类型：`May`
- 外部类型：`MemPage`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Number`
- 外部类型：`Otherwise`
- 外部类型：`Page`
- 外部类型：`Pgno`
- 外部类型：`Return`
- 外部类型：`SQLite`
- 外部类型：`The`
- 外部类型：`There`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-63efcf7efb` → 本草稿移入 `cases/defect/auto-sqlite-63efcf7efb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
