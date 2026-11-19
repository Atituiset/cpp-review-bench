# auto-sqlite-1f8fd5ee54

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#13aaeee46de74ae7e74f9905837c242bbfd1d10f](https://github.com/sqlite/sqlite/commit/13aaeee46de74ae7e74f9905837c242bbfd1d10f) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #13aaeee46de74ae7e74f9905837c242bbfd1d10f (https://github.com/sqlite/sqlite/commit/13aaeee46de74ae7e74f9905837c242bbfd1d10f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 13aaeee46de74ae7e74f9905837c242bbfd1d10f Disable the before-trigger-read optimization if the trigger uses a :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -954,6 +954,20 @@ void sqlite3Update(
   newmask = sqlite3TriggerColmask(
       pParse, pTrigger, pChanges, 1, TRIGGER_BEFORE, pTab, onError
   );
+#ifndef SQLITE_OMIT_GENERATED_COLUMNS
+  if( newmask!=0 && (pTab->tabFlags & TF_HasGenerated)!=0 ){
+    /* If a BEFORE trigger reads a generated column via new.*, then we do
+    ** not know exactly which columns are needed, so load them all */
+    for(i=0; i<pTab->nCol; i++){
+      if( (pTab->aCol[i].colFlags & COLFLAG_GENERATED)!=0
+       && (newmask & MASKBIT32(i<31 ? i : 31))!=0
+      ){
+        newmask = 0xffffffff;
+        break;
+      }
+    }
+  }
+#endif
   for(i=0, k=regNew; i<pTab->nCol; i++, k++){
     if( i==pTab->iPKey ){
       sqlite3VdbeAddOp2(v, OP_Null, 0, k);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3TriggerColmask`
- 外部函数：`sqlite3VdbeAddOp2`
- 大写宏：`TRIGGER_BEFORE`
- 外部类型：`OP_Null`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1f8fd5ee54` → 本草稿移入 `cases/defect/auto-sqlite-1f8fd5ee54/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
