# auto-linux-9de3fdbb05

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f802b6558eb81bd5888d761a0cdc8afe5fd8c941](https://github.com/torvalds/linux/commit/f802b6558eb81bd5888d761a0cdc8afe5fd8c941) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 10 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f802b6558eb81bd5888d761a0cdc8afe5fd8c941 (https://github.com/torvalds/linux/commit/f802b6558eb81bd5888d761a0cdc8afe5fd8c941)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 493；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f802b6558eb81bd5888d761a0cdc8afe5fd8c941 Merge tag 'xfs-fixes-7.4-rc4' of git://git.kernel.org:/pub/scm/fs/xfs/xfs-linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -490,15 +490,14 @@ xrep_tempfile_copyin(
 	struct xfs_buf		*bp;
 	xfs_fileoff_t		flush_mask;
 	xfs_fileoff_t		end = off + len;
-	loff_t			pos = XFS_FSB_TO_B(mp, off);
 	int			error = 0;
 
 	ASSERT(S_ISREG(VFS_I(sc->tempip)->i_mode));
 
 	/* Flush buffers to disk every 512K */
 	flush_mask = XFS_B_TO_FSBT(mp, (1U << 19)) - 1;
 
-	for (; off < end; off++, pos += mp->m_sb.sb_blocksize) {
+	for (; off < end; off++) {
 		struct xfs_bmbt_irec	map;
 		int			nmaps = 1;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`ASSERT`
- 大写宏：`S_ISREG`
- 大写宏：`VFS_I`
- 大写宏：`XFS_B_TO_FSBT`
- 大写宏：`XFS_FSB_TO_B`
- 外部类型：`Flush`
- 外部类型：`loff_t`
- 外部类型：`xfs_bmbt_irec`
- 外部类型：`xfs_buf`
- 外部类型：`xfs_fileoff_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-9de3fdbb05` → 本草稿移入 `cases/defect/auto-linux-9de3fdbb05/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
