# auto-linux-0b48002e7b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a804c5742bff224d4cb4d25dd383554d598be32b](https://github.com/torvalds/linux/commit/a804c5742bff224d4cb4d25dd383554d598be32b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 90 |
| 编译错误数（gcc syntax-only） | 77（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a804c5742bff224d4cb4d25dd383554d598be32b (https://github.com/torvalds/linux/commit/a804c5742bff224d4cb4d25dd383554d598be32b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 661；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a804c5742bff224d4cb4d25dd383554d598be32b Merge tag 'lsm-pr-20261016' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/lsm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -658,7 +658,7 @@ int inode_permission(const struct mnt_idmap *idmap,
 	if (unlikely(retval))
 		return retval;
 
-	return security_inode_permission(inode, mask);
+	return security_inode_permission(idmap, inode, mask);
 }
 EXPORT_SYMBOL(inode_permission);
 
@@ -695,7 +695,7 @@ static __always_inline int lookup_inode_permission_may_exec(const struct mnt_idm
 	if (unlikely(((inode->i_mode & 0111) != 0111) || !no_acl_inode(inode)))
 		return inode_permission(idmap, inode, mask);
 
-	return security_inode_permission(inode, mask);
+	return security_inode_permission(idmap, inode, mask);
 }
 
 /**
@@ -4188,7 +4188,7 @@ int vfs_create(const struct mnt_idmap *idmap, struct dentry *dentry, umode_t mod
 		return -EACCES;	/* shouldn't it be ENOSYS? */
 
 	mode = vfs_prepare_mode(idmap, dir, mode, S_IALLUGO, S_IFREG);
-	error = security_inode_create(dir, dentry, mode);
+	error = security_inode_create(idmap, dir, dentry, mode);
 	if (error)
 		return error;
 	error = try_break_deleg(dir, LEASE_BREAK_DIR_CREATE, di);
@@ -4212,7 +4212,7 @@ int vfs_mkobj(struct dentry *dentry, umode_t mode,
 
 	mode &= S_IALLUGO;
 	mode |= S_IFREG;
-	error = security_inode_create(dir, dentry, mode);
+	error = security_inode_create(&nop_mnt_idmap, dir, dentry, mode);
 	if (error)
 		return error;
 	error = f(dentry, mode, arg);
@@ -4328,7 +4328,7 @@ static int may_o_create(const struct mnt_idmap *idmap,
 	if (error)
 		return error;
 
-	return security_inode_create(dir->dentry->d_inode, dentry, mode);
+	return security_inode_create(idmap, dir->dentry->d_inode, dentry, mode);
 }
 
 /**
@@ -5268,7 +5268,7 @@ int vfs_mknod(const struct mnt_idmap *idmap, struct inode *dir,
 	if (error)
 		return error;
 
-	error = security_inode_mknod(dir, dentry, mode, dev);
+	error = security_inode_mknod(idmap, dir, dentry, mode, dev);
 	if (error)
 		return error;
 
@@ -5405,7 +5405,7 @@ struct dentry *vfs_mkdir(const struct mnt_idmap *idmap, struct inode *dir,
 		goto err;
 
 	mode = vfs_prepare_mode(idmap, dir, mode, S_IRWXUGO | S_ISVTX, S_IFDIR);
-	error = security_inode_mkdir(dir, dentry, mode);
+	error = security_inode_mkdir(idmap, dir, dentry, mode);
 	if (error)
 		goto err;
 
@@ -5793,7 +5793,7 @@ int vfs_symlink(const struct mnt_idmap *idmap, struct inode *dir,
 	if (!dir->i_op->symlink)
 		return -EPERM;
 
-	error = security_inode_symlink(dir, dentry, oldname);
+	error = security_inode_symlink(idmap, dir, dentry, oldname);
 	if (error)
 		return error;
 
@@ -5917,7 +5917,7 @@ int vfs_link(struct dentry *old_dentry, const struct mnt_idmap *idmap,
 	if (S_ISDIR(inode->i_mode))
 		return -EPERM;
 
-	error = security_inode_link(old_dentry, dir, new_dentry);
+	error = security_inode_link(idmap, old_dentry, dir, new_dentry);
 	if (error)
 		return error;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`capable_wrt_inode_uidgid`
- 外部函数：`current_fsuid`
- 外部函数：`devcgroup_inode_permission`
- 外部函数：`get_cached_acl_rcu`
- 外部函数：`get_inode_acl`
- 外部函数：`i_gid_into_vfsgid`
- 外部函数：`i_uid_into_vfsuid`
- 外部函数：`is_uncached_acl`
- 外部函数：`likely`
- 外部函数：`mode_strip_sgid`
- 外部函数：`mode_strip_umask`
- 外部函数：`permission`
- 外部函数：`posix_acl_permission`
- 外部函数：`posix_acl_release`
- 外部函数：`sb_rdonly`
- 外部函数：`security_inode_permission`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`unlikely`
- 外部函数：`vfsgid_in_group_p`
- 外部函数：`vfsuid_eq_kuid`
- 大写宏：`ACL`
- 大写宏：`ACL_TYPE_ACCESS`
- 大写宏：`CAP_DAC_OVERRIDE`
- 大写宏：`CAP_DAC_READ_SEARCH`
- 大写宏：`CONFIG_FS_POSIX_ACL`
- 大写宏：`EACCES`
- 大写宏：`EAGAIN`
- 大写宏：`ECHILD`
- 大写宏：`EPERM`
- 大写宏：`EROFS`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`HAS_UNMAPPED_ID`
- 大写宏：`IOP_FASTPERM`
- 大写宏：`IS_ERR`
- 大写宏：`IS_IMMUTABLE`
- 大写宏：`IS_POSIXACL`
- 大写宏：`MAY_EXEC`
- 大写宏：`MAY_NOT_BLOCK`
- 大写宏：`MAY_READ`
- 大写宏：`MAY_WRITE`
- 大写宏：`PTR_ERR`
- 大写宏：`RCU`
- 大写宏：`READ_ONCE`
- 大写宏：`RWX`
- 大写宏：`S_IFMT`
- 大写宏：`S_IRWXG`
- 大写宏：`S_ISDIR`
- 大写宏：`S_ISLNK`
- 大写宏：`S_ISREG`
- 大写宏：`S_IXUGO`
- 外部类型：`ACLs`
- 外部类型：`After`
- 外部类型：`Apply`
- 外部类型：`Are`
- 外部类型：`Bits`
- 外部类型：`Common`
- 外部类型：`DACs`
- 外部类型：`Do`
- 外部类型：`Executable`
- 外部类型：`If`
- 外部类型：`Need`
- 外部类型：`No`
- 外部类型：`Nobody`
- 外部类型：`Only`
- 外部类型：`Read`
- 外部类型：`Searching`
- 外部类型：`This`
- 外部类型：`Updating`
- 外部类型：`inode`
- 外部类型：`mnt_idmap`
- 外部类型：`posix_acl`
- 外部类型：`super_block`
- 外部类型：`umode_t`
- 外部类型：`vfsgid_t`
- 外部类型：`vfsuid_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0b48002e7b` → 本草稿移入 `cases/defect/auto-linux-0b48002e7b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
