# auto-linux-d8913e7f7c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#eef2bc5a82c25d25c3082f5d86d97a300caeb678](https://github.com/torvalds/linux/commit/eef2bc5a82c25d25c3082f5d86d97a300caeb678) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eef2bc5a82c25d25c3082f5d86d97a300caeb678 (https://github.com/torvalds/linux/commit/eef2bc5a82c25d25c3082f5d86d97a300caeb678)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eef2bc5a82c25d25c3082f5d86d97a300caeb678 Merge tag 'landlock-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/mic/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -22,10 +22,10 @@
  * enacted.  Vice versa, access rights that are not specifically listed here are
  * not going to be denied by this ruleset when it is enacted.
  *
- * For historical reasons, the %LANDLOCK_ACCESS_FS_REFER right is always denied
- * by default, even when its bit is not set in @handled_access_fs.  In order to
- * add new rules with this access right, the bit must still be set explicitly
- * (cf. `Filesystem flags`_).
+ * For historical reasons, as soon as any filesystem access right is handled in
+ * @handled_access_fs, the %LANDLOCK_ACCESS_FS_REFER right is always denied as
+ * well.  In order to add new rules with this access right, the bit must still
+ * be set explicitly (cf. `Filesystem flags`_).
  *
  * The explicit listing of *handled access rights* is required for backwards
  * compatibility reasons.  In most use cases, processes that use Landlock will
@@ -462,10 +462,11 @@ struct landlock_capability_attr {
  *   This access right is available since the second version of the Landlock
  *   ABI.
  *
- *   This is the only access right which is denied by default by any ruleset,
- *   even if the right is not specified as handled at ruleset creation time.
- *   The only way to make a ruleset grant this right is to explicitly allow it
- *   for a specific directory by adding a matching rule to the ruleset.
+ *   This is the only access right which is denied by default by any ruleset
+ *   that handles filesystem access rights, even if the right is not specified
+ *   as handled at ruleset creation time.  The only way to make a ruleset grant
+ *   this right is to explicitly allow it for a specific directory by adding a
+ *   matching rule to the ruleset.
  *
  *   In particular, when using the first Landlock ABI version, Landlock will
  *   always deny attempts to reparent files between different directories.
@@ -622,10 +623,9 @@ struct landlock_capability_attr {
  * - %LANDLOCK_PERMISSION_CAPABILITY_USE: Restrict the use of specific Linux
  *   capabilities.  A process in a Landlock domain that handles this permission
  *   is denied from exercising capabilities that are not explicitly allowed by a
- *   %LANDLOCK_RULE_CAPABILITY rule.  This hook is purely restrictive: it can
- *   deny capabilities that the kernel would otherwise grant, but it can never
- *   grant capabilities that the kernel already denied.  Support added in
- *   Landlock ABI version 12.
+ *   %LANDLOCK_RULE_CAPABILITY rule.  Landlock only reduces the capabilities
+ *   that processes can use: it does not let them use capabilities that they
+ *   would not have without Landlock.  Support added in Landlock ABI version 12.
  */
 #define LANDLOCK_PERMISSION_NAMESPACE_USE			(1ULL << 0)
 #define LANDLOCK_PERMISSION_CAPABILITY_USE			(1ULL << 1)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`ABI`
- 大写宏：`LANDLOCK_ACCESS_FS_REFER`
- 大写宏：`LANDLOCK_RULE_CAPABILITY`
- 外部类型：`Filesystem`
- 外部类型：`For`
- 外部类型：`In`
- 外部类型：`Landlock`
- 外部类型：`Linux`
- 外部类型：`Restrict`
- 外部类型：`Support`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Vice`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d8913e7f7c` → 本草稿移入 `cases/defect/auto-linux-d8913e7f7c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
