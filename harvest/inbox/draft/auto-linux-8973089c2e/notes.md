# auto-linux-8973089c2e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#aacb7797504183a90d66b19b1efb555cfe9b3eec](https://github.com/torvalds/linux/commit/aacb7797504183a90d66b19b1efb555cfe9b3eec) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 75 |
| 编译错误数（gcc syntax-only） | 63（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #aacb7797504183a90d66b19b1efb555cfe9b3eec (https://github.com/torvalds/linux/commit/aacb7797504183a90d66b19b1efb555cfe9b3eec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 371；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT aacb7797504183a90d66b19b1efb555cfe9b3eec Merge tag 'ovl-fixes-7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/overlayfs/vfs :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -368,7 +368,7 @@ static bool ovl_type_origin(struct dentry *dentry)
 }
 
 static int ovl_create_upper(struct dentry *dentry, struct inode *inode,
-			    struct ovl_cattr *attr)
+			    struct ovl_cattr *attr, bool origin)
 {
 	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
 	struct dentry *upperdir = ovl_dentry_upper(dentry->d_parent);
@@ -393,7 +393,7 @@ static int ovl_create_upper(struct dentry *dentry, struct inode *inode,
 		ovl_set_opaque(dentry, newdentry);
 	}
 
-	ovl_dir_modified(dentry->d_parent, false);
+	ovl_dir_modified(dentry->d_parent, origin);
 	err = ovl_instantiate(dentry, inode, newdentry, !!attr->hardlink, NULL);
 	if (err)
 		goto out_cleanup;
@@ -491,7 +491,7 @@ static int ovl_set_upper_acl(struct ovl_fs *ofs, struct dentry *upperdentry,
 }
 
 static int ovl_create_over_whiteout(struct dentry *dentry, struct inode *inode,
-				    struct ovl_cattr *cattr)
+				    struct ovl_cattr *cattr, bool origin)
 {
 	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
 	struct dentry *workdir = ovl_workdir(dentry);
@@ -582,7 +582,7 @@ static int ovl_create_over_whiteout(struct dentry *dentry, struct inode *inode,
 		if (err)
 			goto out_cleanup_unlocked;
 	}
-	ovl_dir_modified(dentry->d_parent, false);
+	ovl_dir_modified(dentry->d_parent, origin);
 	err = ovl_instantiate(dentry, inode, newdentry, hardlink, NULL);
 	if (err) {
 		ovl_cleanup(ofs, upperdir, newdentry);
@@ -647,12 +647,12 @@ DEFINE_CLASS(ovl_override_creator_creds,
 
 static int ovl_create_handle_whiteouts(struct dentry *dentry,
 				       struct inode *inode,
-				       struct ovl_cattr *attr)
+				       struct ovl_cattr *attr, bool origin)
 {
 	if (!ovl_dentry_is_whiteout(dentry))
-		return ovl_create_upper(dentry, inode, attr);
+		return ovl_create_upper(dentry, inode, attr, origin);
 
-	return ovl_create_over_whiteout(dentry, inode, attr);
+	return ovl_create_over_whiteout(dentry, inode, attr, origin);
 }
 
 static int ovl_create_or_link(struct dentry *dentry, struct inode *inode,
@@ -687,12 +687,14 @@ static int ovl_create_or_link(struct dentry *dentry, struct inode *inode,
 		 */
 
 		if (attr->hardlink)
-			return ovl_create_handle_whiteouts(dentry, inode, attr);
+			return ovl_create_handle_whiteouts(dentry, inode, attr,
+							   origin);
 
 		scoped_class(ovl_override_creator_creds, cred, original_creds, dentry, inode, attr->mode) {
 			if (IS_ERR(cred))
 				return PTR_ERR(cred);
-			return ovl_create_handle_whiteouts(dentry, inode, attr);
+			return ovl_create_handle_whiteouts(dentry, inode, attr,
+							   origin);
 		}
 	}
 	return err;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cache`
- 外部函数：`cases`
- 外部函数：`d_drop`
- 外部函数：`d_inode`
- 外部函数：`d_instantiate`
- 外部函数：`d_instantiate_new`
- 外部函数：`d_is_dir`
- 外部函数：`d_mark_tmpfile`
- 外部函数：`dget`
- 外部函数：`dput`
- 外部函数：`end_removing`
- 外部函数：`failed`
- 外部函数：`inc_nlink`
- 外部函数：`inode_init_owner`
- 外部函数：`no_free_ptr`
- 外部函数：`override_creds`
- 外部函数：`ovl_check_setxattr`
- 外部函数：`ovl_create_real`
- 外部函数：`ovl_creds`
- 外部函数：`ovl_dentry_init_reval`
- 外部函数：`ovl_dentry_is_whiteout`
- 外部函数：`ovl_dentry_revalidate_common`
- 外部函数：`ovl_dentry_set_opaque`
- 外部函数：`ovl_dentry_set_upper_alias`
- 外部函数：`ovl_dentry_upper`
- 外部函数：`ovl_dir_modified`
- 外部函数：`ovl_do_rmdir`
- 外部函数：`ovl_do_unlink`
- 外部函数：`ovl_get_inode`
- 外部函数：`ovl_get_inode_locked`
- 外部函数：`ovl_inode_real`
- 外部函数：`ovl_link`
- 外部函数：`ovl_mkdir`
- 外部函数：`ovl_obtain_alias`
- 外部函数：`ovl_rename`
- 外部函数：`ovl_set_flag`
- 外部函数：`ovl_set_impure`
- 外部函数：`ovl_workdir`
- 外部函数：`pr_err`
- 外部函数：`pr_warn_ratelimited`
- 外部函数：`security_dentry_create_files_as`
- 外部函数：`start_removing_dentry`
- 大写宏：`CLASS`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`EXDEV`
- 大写宏：`IS_ERR`
- 大写宏：`NULL`
- 大写宏：`OVL_FS`
- 大写宏：`OVL_UPPERDATA`
- 大写宏：`OVL_XATTR_OPAQUE`
- 大写宏：`PTR_ERR`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`XXX`
- 外部类型：`And`
- 外部类型：`Because`
- 外部类型：`But`
- 外部类型：`Fail`
- 外部类型：`Force`
- 外部类型：`In`
- 外部类型：`Set`
- 外部类型：`So`
- 外部类型：`Unless`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`cred`
- 外部类型：`dentry`
- 外部类型：`file`
- 外部类型：`inode`
- 外部类型：`ovl_cattr`
- 外部类型：`ovl_fs`
- 外部类型：`ovl_inode_params`
- 外部类型：`umode_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8973089c2e` → 本草稿移入 `cases/defect/auto-linux-8973089c2e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
