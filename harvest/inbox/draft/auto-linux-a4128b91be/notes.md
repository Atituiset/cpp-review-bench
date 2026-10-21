# auto-linux-a4128b91be

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#331eadd85f14bac85663c14c99228f6dedf64d87](https://github.com/torvalds/linux/commit/331eadd85f14bac85663c14c99228f6dedf64d87) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #331eadd85f14bac85663c14c99228f6dedf64d87 (https://github.com/torvalds/linux/commit/331eadd85f14bac85663c14c99228f6dedf64d87)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 251；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 331eadd85f14bac85663c14c99228f6dedf64d87 Merge tag 'selinux-pr-20261015' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/selinux :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -75,8 +75,8 @@ struct selinux_fs_info {
 	char **bool_pending_names;
 	int *bool_pending_values;
 	struct dentry *class_dir;
-	unsigned long last_class_ino;
-	unsigned long last_ino;
+	u64 last_class_ino;
+	u64 last_ino;
 	struct super_block *sb;
 };
 
@@ -247,11 +247,6 @@ static int sel_mmap_handle_status(struct file *filp,
 	/* only allows one page from the head */
 	if (vma->vm_pgoff > 0 || size != PAGE_SIZE)
 		return -EIO;
-	/* disallow writable mapping */
-	if (vma->vm_flags & VM_WRITE)
-		return -EPERM;
-	/* disallow mprotect() turns it into writable */
-	vm_flags_clear(vma, VM_MAYWRITE);
 
 	return remap_pfn_range(vma, vma->vm_start,
 			       page_to_pfn(status),
@@ -304,15 +299,14 @@ static int sel_make_bools(struct selinux_policy *newpolicy, struct dentry *bool_
 			  int **bool_pending_values);
 static int sel_make_classes(struct selinux_policy *newpolicy,
 			    struct dentry *class_dir,
-			    unsigned long *last_class_ino);
+			    u64 *last_class_ino);
 
 /* declaration for sel_make_class_dirs */
 static struct dentry *sel_make_dir(struct dentry *dir, const char *name,
-			unsigned long *ino);
+				   u64 *ino);
 
 /* declaration for sel_make_policy_nodes */
-static struct dentry *sel_make_swapover_dir(struct super_block *sb,
-						unsigned long *ino);
+static struct dentry *sel_make_swapover_dir(struct super_block *sb, u64 *ino);
 
 static ssize_t sel_read_mls(struct file *filp, char __user *buf,
 				size_t count, loff_t *ppos)
@@ -467,7 +461,7 @@ static int sel_make_policy_nodes(struct selinux_fs_info *fsi,
 	unsigned int bool_num = 0;
 	char **bool_names = NULL;
 	int *bool_values = NULL;
-	unsigned long tmp_ino = fsi->last_ino; /* Don't increment last_ino in this function */
+	u64 tmp_ino = fsi->last_ino; /* Don't increment last_ino in this function */
 
 	tmp_parent = sel_make_swapover_dir(fsi->sb, &tmp_ino);
 	if (IS_ERR(tmp_parent))
@@ -1568,30 +1562,32 @@ static int sel_make_initcon_files(struct dentry *dir)
 	return err;
 }
 
-static inline unsigned long sel_class_to_ino(u16 class)
+static inline u64 sel_class_to_ino(u16 class)
 {
 	return (class * (SEL_VEC_MAX + 1)) | SEL_CLASS_INO_OFFSET;
 }
 
-static inline u16 sel_ino_to_class(unsigned long ino)
+static inline u16 sel_ino_to_class(u64 ino)
 {
-	return (ino & SEL_INO_MASK) / (SEL_VEC_MAX + 1);
+	u32 ino_masked = ino & SEL_INO_MASK;
+	return ino_masked / (SEL_VEC_MAX + 1);
 }
 
-static inline unsigned long sel_perm_to_ino(u16 class, u32 perm)
+static inline u64 sel_perm_to_ino(u16 class, u32 perm)
 {
 	return (class * (SEL_VEC_MAX + 1) + perm) | SEL_CLASS_INO_OFFSET;
 }
 
-static inline u32 sel_ino_to_perm(unsigned long ino)
+static inline u32 sel_ino_to_perm(u64 ino)
 {
-	return (ino & SEL_INO_MASK) % (SEL_VEC_MAX + 1);
+	u32 ino_masked = ino & SEL_INO_MASK;
+	return ino_masked % (SEL_VEC_MAX + 1);
 }
 
 static ssize_t sel_read_class(struct file *file, char __user *buf,
 				size_t count, loff_t *ppos)
 {
-	unsigned long ino = file_inode(file)->i_ino;
+	u64 ino = file_inode(file)->i_ino;
 	char res[TMPBUFLEN];
 	ssize_t len = scnprintf(res, sizeof(res), "%d", sel_ino_to_class(ino));
 	return simple_read_from_buffer(buf, count, ppos, res, len);
@@ -1605,7 +1601,7 @@ static const struct file_operations sel_class_ops = {
 static ssize_t sel_read_perm(struct file *file, char __user *buf,
 				size_t count, loff_t *ppos)
 {
-	unsigned long ino = file_inode(file)->i_ino;
+	u64 ino = file_inode(file)->i_ino;
 	char res[TMPBUFLEN];
 	ssize_t len = scnprintf(res, sizeof(res), "%d", sel_ino_to_perm(ino));
 	return simple_read_from_buffer(buf, count, ppos, res, len);
@@ -1622,7 +1618,7 @@ static ssize_t sel_read_policycap(struct file *file, char __user *buf,
 	int value;
 	char tmpbuf[TMPBUFLEN];
 	ssize_t length;
-	unsigned long i_ino = file_inode(file)->i_ino;
+	u64 i_ino = file_inode(file)->i_ino;
 
 	value = security_policycap_supported(i_ino & SEL_INO_MASK);
 	length = scnprintf(tmpbuf, TMPBUFLEN, "%d", value);
@@ -1698,7 +1694,7 @@ static int sel
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`file_inode`
- 外部函数：`mprotect`
- 外部函数：`page_to_pfn`
- 外部函数：`remap_pfn_range`
- 外部函数：`scnprintf`
- 外部函数：`security_mls_enabled`
- 外部函数：`security_policycap_supported`
- 外部函数：`simple_read_from_buffer`
- 外部函数：`vm_flags_clear`
- 大写宏：`EIO`
- 大写宏：`EPERM`
- 大写宏：`IS_ERR`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`SEL_VEC_MAX`
- 大写宏：`VM_MAYWRITE`
- 大写宏：`VM_WRITE`
- 外部类型：`Don`
- 外部类型：`dentry`
- 外部类型：`file`
- 外部类型：`fs_context`
- 外部类型：`inode`
- 外部类型：`loff_t`
- 外部类型：`mnt_idmap`
- 外部类型：`selinux_policy`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`super_block`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a4128b91be` → 本草稿移入 `cases/defect/auto-linux-a4128b91be/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
