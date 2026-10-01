# auto-postgres-f90a9b3d56

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b69356cd789fe963447177a0077c12c6c322c203](https://github.com/postgres/postgres/commit/b69356cd789fe963447177a0077c12c6c322c203) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b69356cd789fe963447177a0077c12c6c322c203 (https://github.com/postgres/postgres/commit/b69356cd789fe963447177a0077c12c6c322c203)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b69356cd789fe963447177a0077c12c6c322c203 Fix autovacuum docs related to widening multixid offsets to 64 bits :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -997,9 +997,7 @@ GetNewMultiXactId(int nmembers, MultiXactOffset *offset)
 	 * against catastrophic data loss due to multixact wraparound.  The basic
 	 * rules are:
 	 *
-	 * If we're past multiVacLimit or the safe threshold for member storage
-	 * space, or we don't know what the safe threshold for member storage is,
-	 * start trying to force autovacuum cycles.
+	 * If we're past multiVacLimit, start trying to force autovacuum cycles.
 	 * If we're past multiWarnLimit, start issuing warnings.
 	 * If we're past multiStopLimit, refuse to create new MultiXactIds.
 	 *
@@ -2167,7 +2165,7 @@ SetMultiXactIdLimit(MultiXactId oldest_datminmxid, Oid oldest_datoid)
 	 * Offsets are 64-bits wide and never wrap around, so we don't need to
 	 * consider them for emergency autovacuum purposes.  But now that we're in
 	 * a consistent state, determine MultiXactState->oldestOffset.  It will be
-	 * used to adjust the freezing cutoff, to keep the offsets disk usage in
+	 * used to adjust the freezing cutoff, to keep the members disk usage in
 	 * check.
 	 */
 	SetOldestOffset();
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errmsg`
- 外部函数：`errmsg_internal`
- 大写宏：`DEBUG1`
- 大写宏：`LOG`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`LW_SHARED`
- 外部类型：`But`
- 外部类型：`Determine`
- 外部类型：`Have`
- 外部类型：`If`
- 外部类型：`Install`
- 外部类型：`It`
- 外部类型：`Look`
- 外部类型：`MultiXact`
- 外部类型：`MultiXactGenLock`
- 外部类型：`MultiXactId`
- 外部类型：`MultiXactIds`
- 外部类型：`MultiXactMemberCtl`
- 外部类型：`MultiXactMemberSlruDesc`
- 外部类型：`MultiXactOffset`
- 外部类型：`MultiXactOffsetCtl`
- 外部类型：`MultiXactOffsetSlruDesc`
- 外部类型：`MultiXactState`
- 外部类型：`MultiXactTruncationLock`
- 外部类型：`Normally`
- 外部类型：`Offsets`
- 外部类型：`PRIu64`
- 外部类型：`PhysicalPageExists`
- 外部类型：`PostgreSQL`
- 外部类型：`Read`
- 外部类型：`SimpleLruReadPage_ReadOnly`
- 外部类型：`The`
- 外部类型：`Those`
- 外部类型：`We`
- 外部类型：`When`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f90a9b3d56` → 本草稿移入 `cases/defect/auto-postgres-f90a9b3d56/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
