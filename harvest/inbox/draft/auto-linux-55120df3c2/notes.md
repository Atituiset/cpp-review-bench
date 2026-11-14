# auto-linux-55120df3c2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#3db1c74a20f0552fadc2c6886a43feee8894fa19](https://github.com/torvalds/linux/commit/3db1c74a20f0552fadc2c6886a43feee8894fa19) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 4 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3db1c74a20f0552fadc2c6886a43feee8894fa19 (https://github.com/torvalds/linux/commit/3db1c74a20f0552fadc2c6886a43feee8894fa19)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3db1c74a20f0552fadc2c6886a43feee8894fa19 Merge tag 'cifs-fixes-7.4-rc3' of https://git.manguebit.org/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -57,6 +57,7 @@ struct cifs_sb_info {
 	atomic_t active;
 	atomic_t mnt_cifs_flags;
 	atomic_t outstanding_rreq;	/* nr of rreqs not yet fully deinitialized */
+	atomic_t umount_aborts;		/* nr of umount -f attempts */
 	struct delayed_work prune_tlinks;
 	struct rcu_head rcu;
 
@@ -68,6 +69,7 @@ struct cifs_sb_info {
 	 * (cifs_autodisable_serverino) in order to match new mounts.
 	 */
 	bool mnt_cifs_serverino_autodisabled;
+	bool exiting;			/* umount -f committed: stop retries */
 	/*
 	 * Available once the mount has completed.
 	 */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部类型：`Available`
- 外部类型：`atomic_t`
- 外部类型：`delayed_work`
- 外部类型：`rcu_head`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-55120df3c2` → 本草稿移入 `cases/defect/auto-linux-55120df3c2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
