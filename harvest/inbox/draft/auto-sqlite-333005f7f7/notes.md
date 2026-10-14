# auto-sqlite-333005f7f7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#5bab74a91ba9d755d8102484e871821e796df77a](https://github.com/sqlite/sqlite/commit/5bab74a91ba9d755d8102484e871821e796df77a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5bab74a91ba9d755d8102484e871821e796df77a (https://github.com/sqlite/sqlite/commit/5bab74a91ba9d755d8102484e871821e796df77a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2169；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5bab74a91ba9d755d8102484e871821e796df77a Merge the latest fixes on branch-3.54 into trunk :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2166,7 +2166,18 @@ case OP_AddImm: {            /* in1 */
 case OP_MustBeInt: {            /* jump0, in1 */
   pIn1 = &aMem[pOp->p1];
   if( (pIn1->flags & MEM_Int)==0 ){
-    applyAffinity(pIn1, SQLITE_AFF_NUMERIC, encoding);
+    if( (pIn1->flags & MEM_Real)!=0 ){
+      sqlite3_int64 ix;
+      if( pIn1->u.r>=-9223372036854775808.0
+       && pIn1->u.r<+9223372036854775808.0
+       && (double)(ix = sqlite3RealToI64(pIn1->u.r))==pIn1->u.r
+      ){
+        pIn1->u.i = ix;
+        MemSetTypeFlag(pIn1, MEM_Int);
+      }
+    }else{
+      applyAffinity(pIn1, SQLITE_AFF_NUMERIC, encoding);
+    }
     if( (pIn1->flags & MEM_Int)==0 ){
       VdbeBranchTaken(1, 2);
       if( pOp->p2==0 ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3Atoi64`
- 外部函数：`sqlite3MemRealValueRC`
- 外部函数：`sqlite3RealSameAsInt`
- 外部函数：`sqlite3RealToI64`
- 外部函数：`sqlite3VdbeIntegerAffinity`
- 外部函数：`sqlite3VdbeMemStringify`
- 外部函数：`testcase`
- 大写宏：`CPU`
- 大写宏：`FALSE`
- 大写宏：`NULL`
- 大写宏：`NUMERIC`
- 大写宏：`OPTIMIZATION`
- 大写宏：`SQLITE_AFF_FLEXNUM`
- 大写宏：`SQLITE_AFF_INTEGER`
- 大写宏：`SQLITE_AFF_NUMERIC`
- 大写宏：`SQLITE_AFF_REAL`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`TEXT`
- 外部类型：`Hence`
- 外部类型：`It`
- 外部类型：`MEM_Int`
- 外部类型：`MEM_IntReal`
- 外部类型：`MEM_Real`
- 外部类型：`MEM_Str`
- 外部类型：`Mem`
- 外部类型：`OP_MustBeInt`
- 外部类型：`Only`
- 外部类型：`The`
- 外部类型：`Ticket`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-333005f7f7` → 本草稿移入 `cases/defect/auto-sqlite-333005f7f7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
