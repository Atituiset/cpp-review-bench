# auto-sqlite-1779be43af

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84](https://github.com/sqlite/sqlite/commit/31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 30（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84 (https://github.com/sqlite/sqlite/commit/31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 579；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31b1a10a9c623fa1078e0a95a8d711e1ae2d0f84 Change the order of fields in an internal structure to avoid the possibility of leaking heap address information when a  :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -99,6 +99,18 @@ typedef struct PGroup PGroup;
 ** object is positioned immediately after the page buffer to serve as an
 ** overrun area, so that overreads are harmless.
 **
+** Under some circumstances, using a strategically corrupt db file, one
+** or two varints may be read from past the end of the page buffer and
+** returned to the user (as rowid values). For this reason, we ensure that
+** the first few elements of PgHdr1 are not pointers. If they were, then
+** a determined attacker might be able to read the value of the pointer,
+** which can be used as part of a larger exploit. Furthermore, because the
+** first two elements of PgHdr1 are isBulkLocal and isAnchor - u16 variables
+** that always contain either 1 or 0 - the PgHdr1 structure always begins
+** with 4 bytes with the high-bit cleared, and so code attempting to read
+** varints can never progress past this point and read any more sensitive
+** information further inside the structure.
+**
 ** Variables isBulkLocal and isAnchor were once type "u8". That works,
 ** but causes a 2-byte gap in the structure for most architectures (since
 ** pointers must be either 4 or 8-byte aligned). As this structure is located
@@ -107,25 +119,41 @@ typedef struct PGroup PGroup;
 ** read part of this structure before the corruption is detected. This
 ** can cause a valgrind error if the uninitialized gap is accessed. Using u16
 ** ensures there is no such gap, and therefore no bytes of uninitialized
-** memory in the structure.
+** memory in the structure. Keeping the u16 type also lets isBulkLocal,
+** isAnchor and iKey fill exactly the 8 bytes (2+2+4) that precede the
+** 8-byte-aligned "page" member, so no uninitialized padding is exposed to
+** an overread.
 **
 ** The pLruNext and pLruPrev pointers form a double-linked circular list
 ** of all pages that are unpinned.  The PGroup.lru element (which should be
 ** the only element on the list with PgHdr1.isAnchor set to 1) forms the
 ** beginning and the end of the list.
 */
 struct PgHdr1 {
-  sqlite3_pcache_page page; /* Base class. Must be first. pBuf & pExtra */
-  unsigned int iKey;        /* Key value (page number) */
   u16 isBulkLocal;          /* This page from bulk local storage */
   u16 isAnchor;             /* This is the PGroup.lru element */
+  unsigned int iKey;        /* Key value (page number) */
+  sqlite3_pcache_page page; /* Base class. pBuf & pExtra. Deliberately not the
+                            ** first member - see the note above. */
   PgHdr1 *pNext;            /* Next in hash table chain */
   PCache1 *pCache;          /* Cache that currently owns this page */
   PgHdr1 *pLruNext;         /* Next in circular LRU list of unpinned pages */
   PgHdr1 *pLruPrev;         /* Previous in LRU list of unpinned pages */
                             /* NB: pLruPrev is only valid if pLruNext!=0 */
 };
 
+/*
+** Given the sqlite3_pcache_page object handed out to the pcache.c layer,
+** recover the PgHdr1 that contains it. PgHdr1.page is no longer required to
+** be the first member of PgHdr1, so this goes through the "page" member
+** rather than casting the structure pointer. The fetch methods produce the
+** other direction directly with &pPage->page, only on paths where pPage is
+** known to be non-NULL. PAGE_TO_PGHDR1() is likewise only ever given a
+** non-NULL page that this module previously handed out.
+*/
+#define PAGE_TO_PGHDR1(pPg) \
+  ((PgHdr1*)(((char*)(pPg)) - offsetof(PgHdr1,page)))
+
 /*
 ** A page is pinned if it is not on the LRU list.  To be "pinned" means
 ** that the page is in active use and must not be deallocated.
@@ -576,7 +604,7 @@ static void pcache1ResizeHash(PCache1 *p){
 **
 ** The PGroup mutex must be held when this function is called.
 */
-static PgHdr1 *pcache1PinPage(PgHdr1 *pPage){
+static sqlite3_pcache_page *pcache1PinPage(PgHdr1 *pPage){
   assert( pPage!=0 );
   assert( PAGE_IS_UNPINNED(pPage) );
   assert( pPage->pLruNext );
@@ -590,7 +618,7 @@ static PgHdr1 *pca
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`architectures`
- 外部函数：`assert`
- 外部函数：`element`
- 外部函数：`offsetof`
- 外部函数：`pcache1FetchNoMutex`
- 外部函数：`pcache1FetchStage2`
- 外部函数：`pcache1PinPage`
- 外部函数：`value`
- 大写宏：`LRU`
- 大写宏：`NULL`
- 大写宏：`SQLITE_NOINLINE`
- 外部类型：`As`
- 外部类型：`Base`
- 外部类型：`Cache`
- 外部类型：`Key`
- 外部类型：`Must`
- 外部类型：`Next`
- 外部类型：`PCache1`
- 外部类型：`PGroup`
- 外部类型：`PgHdr1`
- 外部类型：`Previous`
- 外部类型：`Steps`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Using`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1779be43af` → 本草稿移入 `cases/defect/auto-sqlite-1779be43af/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
