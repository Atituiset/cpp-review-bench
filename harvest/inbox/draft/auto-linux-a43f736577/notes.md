# auto-linux-a43f736577

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#65e4fd942f72a9dba0e4e257290c4fa1aa756854](https://github.com/torvalds/linux/commit/65e4fd942f72a9dba0e4e257290c4fa1aa756854) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 48 |
| 编译错误数（gcc syntax-only） | 41（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #65e4fd942f72a9dba0e4e257290c4fa1aa756854 (https://github.com/torvalds/linux/commit/65e4fd942f72a9dba0e4e257290c4fa1aa756854)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4183；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 65e4fd942f72a9dba0e4e257290c4fa1aa756854 Merge tag 'selinux-pr-20261119' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/selinux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4179,8 +4179,11 @@ static int selinux_mmap_file(struct file *file,
 			     unsigned long reqprot __always_unused,
 			     unsigned long prot, unsigned long flags)
 {
+	const unsigned long map_type = (flags & MAP_TYPE);
+
 	return selinux_mmap_file_common(file, prot,
-					(flags & MAP_TYPE) == MAP_SHARED,
+					(map_type == MAP_SHARED ||
+					 map_type == MAP_SHARED_VALIDATE),
 					false);
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`avc_has_perm`
- 外部函数：`backing_file_user_path`
- 外部函数：`bpf_map_fmode_to_av`
- 外部函数：`current_cred`
- 外部函数：`current_sid`
- 外部函数：`d_inode`
- 外部函数：`file_inode`
- 外部函数：`selinux_backing_file`
- 外部函数：`selinux_bpf_map_security`
- 外部函数：`selinux_bpf_prog_security`
- 外部函数：`selinux_cred`
- 外部函数：`selinux_file`
- 外部函数：`selinux_inode`
- 外部函数：`unlikely`
- 大写宏：`BPF__PROG_RUN`
- 大写宏：`CONFIG_BPF_SYSCALL`
- 大写宏：`EIO`
- 大写宏：`FD__USE`
- 大写宏：`FILE__EXECUTE`
- 大写宏：`FILE__MAP`
- 大写宏：`FILE__READ`
- 大写宏：`FILE__WRITE`
- 大写宏：`FMODE_BACKING`
- 大写宏：`IS_PRIVATE`
- 大写宏：`LSM_AUDIT_DATA_FILE`
- 大写宏：`LSM_AUDIT_DATA_PATH`
- 大写宏：`MAP_SHARED`
- 大写宏：`MAP_TYPE`
- 大写宏：`NULL`
- 大写宏：`PROCESS__EXECMEM`
- 大写宏：`PROT_EXEC`
- 大写宏：`PROT_WRITE`
- 大写宏：`SECCLASS_BPF`
- 大写宏：`SECCLASS_FD`
- 大写宏：`SECCLASS_PROCESS`
- 大写宏：`WARN_ON`
- 外部类型：`We`
- 外部类型：`backing_file_security_struct`
- 外部类型：`bpf_map`
- 外部类型：`bpf_prog`
- 外部类型：`bpf_security_struct`
- 外部类型：`common_audit_data`
- 外部类型：`cred`
- 外部类型：`cred_security_struct`
- 外部类型：`file`
- 外部类型：`file_security_struct`
- 外部类型：`inode`
- 外部类型：`inode_security_struct`
- 外部类型：`path`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a43f736577` → 本草稿移入 `cases/defect/auto-linux-a43f736577/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
