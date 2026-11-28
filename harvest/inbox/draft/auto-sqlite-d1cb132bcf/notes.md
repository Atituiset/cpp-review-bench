# auto-sqlite-d1cb132bcf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#4460306eb112cda8b85a70e6e297934a8e477cec](https://github.com/sqlite/sqlite/commit/4460306eb112cda8b85a70e6e297934a8e477cec) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4460306eb112cda8b85a70e6e297934a8e477cec (https://github.com/sqlite/sqlite/commit/4460306eb112cda8b85a70e6e297934a8e477cec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 933；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4460306eb112cda8b85a70e6e297934a8e477cec New conditional in the page cache with SQLITE_ENABLE_MEMORY_MANAGEMENT :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -930,7 +930,13 @@ static SQLITE_NOINLINE sqlite3_pcache_page *pcache1FetchStage2(
   /* Step 4. Try to recycle a page. */
   if( pCache->bPurgeable
    && !pGroup->lru.pLruPrev->isAnchor
-   && ((pCache->nPage+1>=pCache->nMax) || pcache1UnderMemoryPressure(pCache))
+   && (  pCache->nPage+1>=pCache->nMax
+#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
+      /* See discussion at bug thread 2026-11-18T20:57:17Z */
+      || pGroup->nPurgeable+1>=pGroup->nMaxPage
+#endif
+      || pcache1UnderMemoryPressure(pCache)
+      )
   ){
     PCache1 *pOther;
     pPage = pGroup->lru.pLruPrev;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`mode`
- 外部函数：`pcache1UnderMemoryPressure`
- 外部函数：`sqlite3HeapNearlyFull`
- 外部函数：`sqlite3_initialize`
- 大写宏：`GLOBAL`
- 大写宏：`PAGECACHE`
- 大写宏：`SQLITE_CONFIG_PAGECACHE`
- 大写宏：`SQLITE_WSD`
- 外部类型：`Above`
- 外部类型：`Bounds`
- 外部类型：`Free`
- 外部类型：`Initial`
- 外部类型：`Mutex`
- 外部类型：`Number`
- 外部类型：`PCache`
- 外部类型：`PCache1`
- 外部类型：`PCacheGlobal`
- 外部类型：`PGroup`
- 外部类型：`PgFreeslot`
- 外部类型：`Size`
- 外部类型：`Step`
- 外部类型：`The`
- 外部类型：`True`
- 外部类型：`Try`
- 外部类型：`Use`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-d1cb132bcf` → 本草稿移入 `cases/defect/auto-sqlite-d1cb132bcf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
