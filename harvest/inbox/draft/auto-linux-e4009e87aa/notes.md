# auto-linux-e4009e87aa

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#58aeff92eebcad28da1f01e6d16b04794f7ea4f2](https://github.com/torvalds/linux/commit/58aeff92eebcad28da1f01e6d16b04794f7ea4f2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #58aeff92eebcad28da1f01e6d16b04794f7ea4f2 (https://github.com/torvalds/linux/commit/58aeff92eebcad28da1f01e6d16b04794f7ea4f2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 58aeff92eebcad28da1f01e6d16b04794f7ea4f2 Merge tag 'vfs-7.4-rc1.shared.super' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -150,6 +150,10 @@ extern int vfs_parse_fs_param_source(struct fs_context *fc,
 				     struct fs_parameter *param);
 extern void fc_drop_locked(struct fs_context *fc);
 
+int get_tree_super(struct fs_context *fc,
+		   int (*test)(struct super_block *, struct fs_context *),
+		   int (*fill_super)(struct super_block *sb,
+				     struct fs_context *fc));
 extern int get_tree_nodev(struct fs_context *fc,
 			 int (*fill_super)(struct super_block *sb,
 					   struct fs_context *fc));
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fc_drop_locked`
- 外部函数：`flags`
- 外部函数：`free`
- 外部函数：`fsmount`
- 外部函数：`get_tree_nodev`
- 外部函数：`int`
- 外部函数：`mount`
- 外部函数：`name`
- 外部函数：`reconfiguration`
- 外部函数：`void`
- 大写宏：`FS_CONTEXT_AWAITING_MOUNT`
- 大写宏：`FS_CONTEXT_AWAITING_RECONF`
- 大写宏：`FS_CONTEXT_CREATE_PARAMS`
- 大写宏：`FS_CONTEXT_CREATING`
- 大写宏：`FS_CONTEXT_FAILED`
- 大写宏：`FS_CONTEXT_FOR_MOUNT`
- 大写宏：`FS_CONTEXT_FOR_RECONFIGURE`
- 大写宏：`FS_CONTEXT_FOR_SUBMOUNT`
- 大写宏：`FS_CONTEXT_RECONFIGURING`
- 大写宏：`FS_CONTEXT_RECONF_PARAMS`
- 大写宏：`LSM`
- 大写宏：`SB_`
- 外部类型：`Awaiting`
- 外部类型：`Coming`
- 外部类型：`Failed`
- 外部类型：`Goes`
- 外部类型：`Insertion`
- 外部类型：`Loading`
- 外部类型：`Logging`
- 外部类型：`Mask`
- 外部类型：`Need`
- 外部类型：`New`
- 外部类型：`Owner`
- 外部类型：`Parameter`
- 外部类型：`Proposed`
- 外部类型：`Reconfiguring`
- 外部类型：`Removal`
- 外部类型：`Superblock`
- 外部类型：`The`
- 外部类型：`Userspace`
- 外部类型：`Value`
- 外部类型：`cred`
- 外部类型：`dentry`
- 外部类型：`file`
- 外部类型：`file_system_type`
- 外部类型：`filename`
- 外部类型：`module`
- 外部类型：`mutex`
- 外部类型：`net`
- 外部类型：`refcount_t`
- 外部类型：`size_t`
- 外部类型：`super_block`
- 外部类型：`user_namespace`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e4009e87aa` → 本草稿移入 `cases/defect/auto-linux-e4009e87aa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
