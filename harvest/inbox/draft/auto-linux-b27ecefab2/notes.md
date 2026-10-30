# auto-linux-b27ecefab2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e](https://github.com/torvalds/linux/commit/0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e (https://github.com/torvalds/linux/commit/0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e Merge tag 'ntfs3_for_7.4' of https://github.com/Paragon-Software-Group/linux-ntfs3 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -20,6 +20,7 @@
 #include "debug.h"
 #include "ntfs.h"
 #include "ntfs_fs.h"
+#include <trace/events/ntfs3.h>
 
 /*
  * cifx, btrfs, exfat, ext4, f2fs use this constant.
@@ -183,6 +184,9 @@ static int ntfs_ioctl_set_volume_label(struct ntfs_sb_info *sbi, u8 __user *buf)
 	if (!capable(CAP_SYS_ADMIN))
 		return -EPERM;
 
+	if (sb_rdonly(sbi->sb))
+		return -EROFS;
+
 	if (copy_from_user(user, buf, FSLABEL_MAX))
 		return -EFAULT;
 
@@ -824,6 +828,8 @@ static ssize_t ntfs_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
 	unsigned int dio_flags;
 	ssize_t ret;
 
+	trace_ntfs3_file_read_iter(iocb, iter);
+
 	ret = check_read_restriction(inode);
 	if (ret)
 		return ret;
@@ -1244,6 +1250,8 @@ static ssize_t ntfs_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
 	loff_t vbo, endbyte;
 	ssize_t ret, err;
 
+	trace_ntfs3_file_write_iter(iocb, from);
+
 	if (!inode_trylock(inode)) {
 		if (iocb->ki_flags & IOCB_NOWAIT)
 			return -EAGAIN;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`capable`
- 外部函数：`copy_from_user`
- 外部函数：`inode_trylock`
- 外部函数：`is_bad_ni`
- 外部函数：`is_dedup`
- 外部函数：`is_encrypted`
- 外部函数：`ntfs3_forced_shutdown`
- 外部函数：`ntfs_i`
- 外部函数：`ntfs_inode_warn`
- 外部函数：`unlikely`
- 大写宏：`CAP_SYS_ADMIN`
- 大写宏：`CONFIG_NTFS3_LZX_XPRESS`
- 大写宏：`EAGAIN`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EPERM`
- 大写宏：`FSLABEL_MAX`
- 大写宏：`IOCB_NOWAIT`
- 大写宏：`NI_FLAG_COMPRESSED_MASK`
- 外部类型：`Avoid`
- 外部类型：`inode`
- 外部类型：`loff_t`
- 外部类型：`ntfs_inode`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b27ecefab2` → 本草稿移入 `cases/defect/auto-linux-b27ecefab2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
