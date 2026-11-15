# auto-linux-fd716cd0f4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#cba88960471ac67635080708b67ed5c2a2e4e9fd](https://github.com/torvalds/linux/commit/cba88960471ac67635080708b67ed5c2a2e4e9fd) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 148 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cba88960471ac67635080708b67ed5c2a2e4e9fd (https://github.com/torvalds/linux/commit/cba88960471ac67635080708b67ed5c2a2e4e9fd)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1556；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cba88960471ac67635080708b67ed5c2a2e4e9fd Merge tag 'ksmbd-for-7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/smb :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,6 +7,7 @@
  */
 
 #include <linux/fs.h>
+#include <linux/dcache.h>
 #include <kunit/visibility.h>
 #include <linux/slab.h>
 #include <linux/string.h>
@@ -1207,7 +1208,7 @@ int smb_inherit_dacl(struct ksmbd_conn *conn,
 	struct smb_acl *parent_pdacl;
 	struct smb_ntsd *parent_pntsd = NULL;
 	struct smb_sid owner_sid, group_sid;
-	struct dentry *parent = path->dentry->d_parent;
+	struct dentry *parent;
 	const struct mnt_idmap *idmap = mnt_idmap(path->mnt);
 	int inherited_flags = 0, flags = 0, i, nt_size = 0, pdacl_size;
 	int rc = 0, pntsd_type, ppntsd_size, acl_len, aces_size;
@@ -1217,8 +1218,14 @@ int smb_inherit_dacl(struct ksmbd_conn *conn,
 	char *aces_base;
 	bool is_dir = S_ISDIR(d_inode(path->dentry)->i_mode);
 
+	parent = dget_parent(path->dentry);
+	if (!d_inode(parent)) {
+		dput(parent);
+		return -ENOENT;
+	}
 	ppntsd_size = ksmbd_vfs_get_sd_xattr(conn, idmap,
 					    parent, &parent_pntsd);
+	dput(parent);
 	if (ppntsd_size <= 0)
 		return -ENOENT;
 
@@ -1459,9 +1466,49 @@ bool smb_inherit_flags(int flags, bool is_dir)
 	return false;
 }
 
+/* Whether the caller's credentials hold the group of an S-1-22-2-<gid> SID */
+static bool smb_sid_in_group(const struct smb_sid *sid)
+{
+	gid_t gid;
+
+	if (sid->num_subauth != sid_unix_groups.num_subauth + 1 ||
+	    compare_sids(sid, &sid_unix_groups))
+		return false;
+	gid = le32_to_cpu(sid->sub_auth[sid->num_subauth - 1]);
+	/*
+	 * ksmbd_override_fsids() builds the caller's credentials with
+	 * make_kgid(&init_user_ns, ...), so the group membership test must
+	 * compare in that space. Applying the mount idmap here (as when
+	 * resolving a SID to an on-disk gid) would mismatch on idmapped
+	 * mounts.
+	 */
+	return in_group_p(make_kgid(&init_user_ns, gid));
+}
+
+/* Whether two SIDs are the same SID */
+static bool smb_sid_equal(const struct smb_sid *a, const struct smb_sid *b)
+{
+	return a->num_subauth == b->num_subauth && !compare_sids(a, b);
+}
+
+/*
+ * compare_sids() compares only the sub-authorities both SIDs have, so it
+ * also matches a shorter or longer SID, such as S-1-3 or S-1-3-4-777
+ */
+static bool smb_sid_is_owner_rights(const struct smb_sid *sid)
+{
+	return sid->num_subauth == sid_owner_rights.num_subauth &&
+	       !compare_sids(sid, &sid_owner_rights);
+}
+
+static unsigned int smb_parent_derived_rights(struct ksmbd_conn *conn,
+					      const struct path *path, int uid,
+					      bool authenticated,
+					      unsigned int want);
+
 int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 			__le32 *pdaccess, __le32 raw_daccess, int uid,
-			bool strict)
+			bool authenticated, bool strict)
 {
 	const struct mnt_idmap *idmap = mnt_idmap(path->mnt);
 	struct smb_ntsd *pntsd = NULL;
@@ -1475,13 +1522,23 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 	int granted = requested;
 	struct smb_ace *ace;
 	int i, found = 0;
-	unsigned int access_bits = 0, denied = 0;
+	unsigned int access_bits = 0, denied = 0, grp_granted = 0;
+	unsigned int grp_allowed = 0, grp_denied = 0, bits;
 	struct smb_ace *others_ace = NULL;
-	bool or_ace = false;
+	bool or_ace = false, obj_deny = false, au_ace = false, member, counted;
+	bool grp_ace = false;
 	struct posix_acl_entry *pa_entry;
 	unsigned int sid_type = SIDOWNER;
 	unsigned short ace_size;
 	bool is_owner, owner_rights = false;
+	bool maximal = *pdaccess & FILE_MAXIMAL_ACCESS_LE;
+	/*
+	 * MAXIMUM_ALLOWED requests that are not strict match every session
+	 * against Authenticated Users and do not count ACEs for the
+	 * caller's groups. Strict ones match SIDs as explicit requests do.
+	 */
+	bool open_max = maximal && !strict;
+	bool authusers = authenticated || open_max;
 	vfsuid_t vfsuid;
 
 	ksmbd_debug(SMB, "check permission using windows acl\n");
@@ -1515,14 +1572,17 @@ int smb_check_perm_dacl(struct ksmbd_conn *conn, const struct path *path,
 		/*
 		 * An empty (present, zero-ACE) DACL grants no access to
 		 * anyone except the object own
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`acl`
- 外部函数：`cpu_to_le16`
- 外部函数：`d_inode`
- 外部函数：`empty`
- 外部函数：`from_vfsgid`
- 外部函数：`from_vfsuid`
- 外部函数：`get_inode_acl`
- 外部函数：`gid_eq`
- 外部函数：`gid_valid`
- 外部函数：`inode_lock`
- 外部函数：`inode_unlock`
- 外部函数：`kfree`
- 外部函数：`kmalloc_objs`
- 外部函数：`ksmbd_debug`
- 外部函数：`ksmbd_vfs_get_sd_xattr`
- 外部函数：`ksmbd_vfs_remove_acl_xattrs`
- 外部函数：`ksmbd_vfs_remove_sd_xattrs`
- 外部函数：`ksmbd_vfs_set_sd_xattr`
- 外部函数：`kzalloc`
- 外部函数：`le16_to_cpu`
- 外部函数：`le32_to_cpu`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`mnt_idmap`
- 外部函数：`notify_change`
- 外部函数：`offsetof`
- 外部函数：`posix_acl_alloc`
- 外部函数：`posix_acl_release`
- 外部函数：`pr_err`
- 外部函数：`pr_err_ratelimited`
- 外部函数：`set_posix_acl`
- 外部函数：`smb_map_generic_desired_access`
- 外部函数：`test_share_config_flag`
- 外部函数：`uid_eq`
- 外部函数：`uid_valid`
- 外部函数：`unlikely`
- 大写宏：`ACCESS_ALLOWED`
- 大写宏：`ACCESS_ALLOWED_ACE_TYPE`
- 大写宏：`ACCESS_DENIED_ACE_TYPE`
- 大写宏：`ACCESS_DENIED_CALLBACK_ACE_TYPE`
- 大写宏：`ACCESS_DENIED_OBJECT_ACE_TYPE`
- 大写宏：`ACE`
- 大写宏：`ACL`
- 大写宏：`ACL_GROUP`
- 大写宏：`ACL_GROUP_OBJ`
- 大写宏：`ACL_MASK`
- 大写宏：`ACL_OTHER`
- 大写宏：`ACL_TYPE_ACCESS`
- 大写宏：`ACL_TYPE_DEFAULT`
- 大写宏：`ACL_USER`
- 大写宏：`ACL_USER_OBJ`
- 大写宏：`ATTR_CTIME`
- 大写宏：`ATTR_GID`
- 大写宏：`ATTR_MODE`
- 大写宏：`ATTR_UID`
- 大写宏：`BUFFER`
- 大写宏：`CIFS_SID_BASE_SIZE`
- 大写宏：`CONFIG_FS_POSIX_ACL`
- 大写宏：`DACL`
- 大写宏：`DACL_AUTO_INHERITED`
- 大写宏：`DACL_AUTO_INHERIT_REQ`
- 大写宏：`DACL_PRESENT`
- 大写宏：`DACL_PROTECTED`
- 大写宏：`DELETE`
- 大写宏：`DOMAIN_USER_RID_LE`
- 大写宏：`DTYP`
- 大写宏：`EACCES`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EXPORT_SYMBOL_IF_KUNIT`
- 大写宏：`FILE_EXECUTE`
- 大写宏：`FILE_EXEC_RIGHTS`
- 大写宏：`FILE_GENERIC_EXECUTE_LE`
- 大写宏：`FILE_MAXIMAL_ACCESS_LE`
- 大写宏：`FILE_READ_ATTRIBUTES`
- 大写宏：`FILE_READ_RIGHTS`
- 大写宏：`FILE_WRITE_RIGHTS`
- 大写宏：`GENERIC_ALL`
- 大写宏：`GENERIC_EXECUTE`
- 大写宏：`GENERIC_READ`
- 大写宏：`GENERIC_READ_FLAGS`
- 大写宏：`GENERIC_WRITE`
- 大写宏：`INHERIT_ONLY_ACE`
- 大写宏：`INVALID_GID`
- 大写宏：`INVALID_UID`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`KGIDT_INIT`
- 大写宏：`KSMBD_DEFAULT_GFP`
- 大写宏：`KSMBD_SHARE_FLAG_ACL_XATTR`
- 大写宏：`KUIDT_INIT`
- 大写宏：`NULL`
- 大写宏：`NUM_AUTHS`
- 大写宏：`READ_CONTROL`
- 大写宏：`RID`
- 大写宏：`SACL`
- 大写宏：`SET_MINIMUM_RIGHTS`
- 大写宏：`SID`
- 大写宏：`SIDOWNER`
- 大写宏：`SIDUNIX_GROUP`
- 大写宏：`SID_MAX_SUB_AUTHORITIES`
- 大写宏：`SMB`
- 大写宏：`S_ISDIR`
- 大写宏：`S_ISREG`
- 大写宏：`VFSGIDT_INIT`
- 大写宏：`VFSUIDT_INIT`
- 大写宏：`WRITE_DAC`
- 外部类型：`ACEs`
- 外部类型：`Also`
- 外部类型：`An`
- 外部类型：`Can`
- 外部类型：`Check`
- 外部类型：`Deny`
- 外部类型：`Error`
- 外部类型：`Group`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Just`
- 外部类型：`Local`
- 外部类型：`Only`
- 外部类型：`Owner`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`Unix`
- 外部类型：`Update`
- 外部类型：`WinACL`
- 外部类型：`dentry`
- 外部类型：`gid_t`
- 外部类型：`iattr`
- 外部类型：`inode`
- 外部类型：`kgid_t`
- 外部类型：`ksmbd_conn`
- 外部类型：`ksmbd_tree_connect`
- 外部类型：`kuid_t`
- 外部类型：`mnt_idmap`
- 外部类型：`path`
- 外部类型：`posix_ace_state_array`
- 外部类型：`posix_acl_entry`
- 外部类型：`posix_acl_state`
- 外部类型：`posix_user_ace_state`
- 外部类型：`smb_ace`
- 外部类型：`smb_acl`
- 外部类型：`smb_fattr`
- 外部类型：`smb_ntsd`
- 外部类型：`smb_sid`
- 外部类型：`uid_t`
- 外部类型：`umode_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-fd716cd0f4` → 本草稿移入 `cases/defect/auto-linux-fd716cd0f4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
