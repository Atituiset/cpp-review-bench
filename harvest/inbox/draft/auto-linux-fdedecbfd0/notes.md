# auto-linux-fdedecbfd0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ca62fc8c6b1841039aee6de1ef03c9863751ec04](https://github.com/torvalds/linux/commit/ca62fc8c6b1841039aee6de1ef03c9863751ec04) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 41（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ca62fc8c6b1841039aee6de1ef03c9863751ec04 (https://github.com/torvalds/linux/commit/ca62fc8c6b1841039aee6de1ef03c9863751ec04)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 146；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ca62fc8c6b1841039aee6de1ef03c9863751ec04 Merge tag 'ovl-fixes-7.3' of git://git.kernel.org/pub/scm/linux/kernel/git/overlayfs/vfs :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -72,6 +72,18 @@ static int ovl_copy_acl(struct ovl_fs *ofs, const struct path *path,
 	return err;
 }
 
+static ssize_t ovl_getxattr_value(const struct path *path, char *name,
+				  char **value, ssize_t *value_size);
+
+/*
+ * Any user can store an empty security.capability, which fails to read with
+ * -EINVAL.  It grants no capability, so treat it as none, like no xattr.
+ */
+static bool ovl_no_fscap(ssize_t err)
+{
+	return err == -ENODATA || err == -EOPNOTSUPP || err == -EINVAL;
+}
+
 int ovl_copy_xattr(struct super_block *sb, const struct path *oldpath, struct dentry *new)
 {
 	struct dentry *old = oldpath->dentry;
@@ -129,30 +141,16 @@ int ovl_copy_xattr(struct super_block *sb, const struct path *oldpath, struct de
 			break;
 		}
 
-retry:
-		size = ovl_do_getxattr(oldpath, name, value, value_size);
-		if (size == -ERANGE)
-			size = ovl_do_getxattr(oldpath, name, NULL, 0);
-
+		size = ovl_getxattr_value(oldpath, name, &value, &value_size);
 		if (size < 0) {
+			if (!strcmp(name, XATTR_NAME_CAPS) && ovl_no_fscap(size)) {
+				error = 0;
+				continue;
+			}
 			error = size;
 			break;
 		}
 
-		if (size > value_size) {
-			void *new;
-
-			new = kvmalloc(size, GFP_KERNEL);
-			if (!new) {
-				error = -ENOMEM;
-				break;
-			}
-			kvfree(value);
-			value = new;
-			value_size = size;
-			goto retry;
-		}
-
 		error = ovl_do_setxattr(OVL_FS(sb), new, name, value, size, 0);
 		if (error) {
 			if (error != -EOPNOTSUPP || ovl_must_copy_xattr(name))
@@ -476,8 +474,12 @@ struct ovl_fh *ovl_get_origin_fh(struct ovl_fs *ofs, struct dentry *origin)
 	 * When lower layer doesn't support export operations store a 'null' fh,
 	 * so we can use the overlay.origin xattr to distignuish between a copy
 	 * up and a pure upper inode.
+	 *
+	 * With index enabled, mount already found that all layers can decode
+	 * file handles.  Do not ask again, because an LSM may change its answer
+	 * about CAP_DAC_READ_SEARCH and index users cannot cope with a 'null' fh.
 	 */
-	if (!ovl_can_decode_fh(origin->d_sb))
+	if (!ofs->config.index && !ovl_can_decode_fh(origin->d_sb))
 		return NULL;
 
 	return ovl_encode_real_fh(ofs, d_inode(origin), false);
@@ -808,9 +810,10 @@ static int ovl_copy_up_workdir(struct ovl_copy_up_ctx *c)
 	rd.old_parent = c->workdir;
 	rd.new_parent = c->destdir;
 	rd.flags = 0;
-	err = start_renaming_dentry(&rd, 0, temp,
+	/* The name may have been taken since lookup, never replace it */
+	err = start_renaming_dentry(&rd, LOOKUP_EXCL, temp,
 				    &QSTR_LEN(c->destname.name, c->destname.len));
-	if (err) {
+	if (err && err != -EEXIST) {
 		/* temp or workdir moved underneath us? map to -EIO */
 		err = -EIO;
 	}
@@ -834,9 +837,9 @@ static int ovl_copy_up_workdir(struct ovl_copy_up_ctx *c)
 
 	if (!c->metacopy)
 		ovl_set_upperdata(inode);
-	ovl_inode_update(inode, temp);
 	if (S_ISDIR(inode->i_mode))
 		ovl_set_flag(OVL_WHITEOUTS, inode);
+	ovl_inode_update(inode, temp);
 out:
 	ovl_end_write(c->dentry);
 
@@ -935,6 +938,7 @@ static int ovl_do_copy_up(struct ovl_copy_up_ctx *c)
 	struct ovl_fs *ofs = OVL_FS(c->dentry->d_sb);
 	struct dentry *origin = c->lowerpath.dentry;
 	struct ovl_fh *fh = NULL;
+	struct name_snapshot name;
 	bool to_index = false;
 
 	/*
@@ -972,12 +976,15 @@ static int ovl_do_copy_up(struct ovl_copy_up_ctx *c)
 		goto out_free_fh;
 	} else {
 		/*
-		 * c->dentry->d_name is stabilzed by ovl_copy_up_start(),
-		 * because if we got here, it means that c->dentry has no upper
-		 * alias and changing ->d_name means going through ovl_rename()
-		 * that will call ovl_copy_up() on source and target dentry.
+		 * ovl_copy_up_start() keeps ovl_rename() away from c->dentry,
+		 * because ovl_rename() calls ovl_copy_up() on source and target
+		 * dentry.  It does not stop d_splice_alias() from moving a
+		 * directory that a lookup found under another name after a
+		 * change to the lower layer.  That move frees an external name,
+		 * so take a snapshot.
 		 */
-		c->destname = c->dentry->d_name
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`d_inode`
- 外部函数：`exportfs_encode_inode_fh`
- 外部函数：`kfree`
- 外部函数：`kvfree`
- 外部函数：`kvmalloc`
- 外部函数：`kzalloc`
- 外部函数：`ovl_can_decode_fh`
- 外部函数：`ovl_clear_flag`
- 外部函数：`ovl_copy_up`
- 外部函数：`ovl_copy_up_start`
- 外部函数：`ovl_do_getxattr`
- 外部函数：`ovl_do_setxattr`
- 外部函数：`ovl_encode_real_fh`
- 外部函数：`ovl_end_write`
- 外部函数：`ovl_inode_update`
- 外部函数：`ovl_origin_uuid`
- 外部函数：`ovl_path_upper`
- 外部函数：`ovl_rename`
- 外部函数：`ovl_set_flag`
- 外部函数：`ovl_set_impure`
- 外部函数：`ovl_set_upperdata`
- 外部函数：`start_renaming_dentry`
- 外部函数：`strcmp`
- 外部函数：`strncmp`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`EIO`
- 大写宏：`ENODATA`
- 大写宏：`ENOMEM`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`ERANGE`
- 大写宏：`ERR_PTR`
- 大写宏：`FILEID_INVALID`
- 大写宏：`GFP_KERNEL`
- 大写宏：`MAX_HANDLE_SZ`
- 大写宏：`NULL`
- 大写宏：`OVL_FH_FID_OFFSET`
- 大写宏：`OVL_FH_FLAG_CPU_ENDIAN`
- 大写宏：`OVL_FH_FLAG_PATH_UPPER`
- 大写宏：`OVL_FH_MAGIC`
- 大写宏：`OVL_FH_VERSION`
- 大写宏：`OVL_FS`
- 大写宏：`OVL_VERIFIED_DIGEST`
- 大写宏：`OVL_WHITEOUTS`
- 大写宏：`O_TMPFILE`
- 大写宏：`QSTR_LEN`
- 大写宏：`S_ISDIR`
- 大写宏：`WARN_ON`
- 大写宏：`XATTR_NAME_CAPS`
- 大写宏：`XATTR_POSIX_ACL_ACCESS`
- 大写宏：`XATTR_POSIX_ACL_DEFAULT`
- 大写宏：`XATTR_SECURITY_PREFIX`
- 大写宏：`XATTR_SECURITY_PREFIX_LEN`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`Should`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`dentry`
- 外部类型：`inode`
- 外部类型：`ovl_fh`
- 外部类型：`ovl_fs`
- 外部类型：`path`
- 外部类型：`ssize_t`
- 外部类型：`super_block`
- 外部类型：`uuid_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-fdedecbfd0` → 本草稿移入 `cases/defect/auto-linux-fdedecbfd0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
