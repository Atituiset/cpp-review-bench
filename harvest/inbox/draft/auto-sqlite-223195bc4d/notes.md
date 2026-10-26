# auto-sqlite-223195bc4d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#1820defd768c50e0ea33476c9b396de7c2c60bc4](https://github.com/sqlite/sqlite/commit/1820defd768c50e0ea33476c9b396de7c2c60bc4) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1820defd768c50e0ea33476c9b396de7c2c60bc4 (https://github.com/sqlite/sqlite/commit/1820defd768c50e0ea33476c9b396de7c2c60bc4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 782；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1820defd768c50e0ea33476c9b396de7c2c60bc4 Merge all recent branch-3.54 fixes into trunk :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -779,8 +779,8 @@ static int fts3PendingTermsAddOne(
   pList = (PendingList *)fts3HashFind(pHash, zToken, nToken);
   if( pList ){
     assert( (i64)pList->nData+(i64)nToken+(i64)sizeof(Fts3HashElem)
-             <= (i64)p->nPendingData );
-    p->nPendingData -= (int)(pList->nData + nToken + sizeof(Fts3HashElem));
+             <= p->nPendingData );
+    p->nPendingData -= pList->nData + nToken + sizeof(Fts3HashElem);
   }
   if( fts3PendingListAppend(&pList, p->iPrevDocid, iCol, iPos, &rc) ){
     if( pList==fts3HashInsert(pHash, zToken, nToken, pList) ){
@@ -793,9 +793,9 @@ static int fts3PendingTermsAddOne(
     }
   }
   if( rc==SQLITE_OK ){
-    assert( (i64)p->nPendingData + pList->nData + nToken
+    assert( p->nPendingData + pList->nData + nToken
              + sizeof(Fts3HashElem) <= 0x3fffffff );
-    p->nPendingData += (int)(pList->nData + nToken + sizeof(Fts3HashElem));
+    p->nPendingData += pList->nData + nToken + sizeof(Fts3HashElem);
   }
   return rc;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`fts3HashFind`
- 外部函数：`fts3HashInsert`
- 外部函数：`sqlite3Fts3PutVarint`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_realloc64`
- 大写宏：`FTS3_VARINT_MAX`
- 大写宏：`OUT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 外部类型：`Allocate`
- 外部类型：`Append`
- 外部类型：`Column`
- 外部类型：`Docid`
- 外部类型：`Fts3HashElem`
- 外部类型：`PendingList`
- 外部类型：`Pointer`
- 外部类型：`Position`
- 外部类型：`Return`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-223195bc4d` → 本草稿移入 `cases/defect/auto-sqlite-223195bc4d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
