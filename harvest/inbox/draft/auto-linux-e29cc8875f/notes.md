# auto-linux-e29cc8875f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a804c5742bff224d4cb4d25dd383554d598be32b](https://github.com/torvalds/linux/commit/a804c5742bff224d4cb4d25dd383554d598be32b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a804c5742bff224d4cb4d25dd383554d598be32b (https://github.com/torvalds/linux/commit/a804c5742bff224d4cb4d25dd383554d598be32b)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 407；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a804c5742bff224d4cb4d25dd383554d598be32b Merge tag 'lsm-pr-20261016' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/lsm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -67,6 +67,7 @@ enum fs_value_type;
 struct watch;
 struct watch_notification;
 struct lsm_ctx;
+struct nsset;
 
 /* Default (no) options for the capable function */
 #define CAP_OPT_NONE 0x0
@@ -80,6 +81,7 @@ struct lsm_ctx;
 
 struct ctl_table;
 struct audit_krule;
+struct ns_common;
 struct user_namespace;
 struct timezone;
 
@@ -404,24 +406,28 @@ int security_inode_init_security(struct inode *inode, struct inode *dir,
 int security_inode_init_security_anon(struct inode *inode,
 				      const struct qstr *name,
 				      const struct inode *context_inode);
-int security_inode_create(struct inode *dir, struct dentry *dentry, umode_t mode);
+int security_inode_create(const struct mnt_idmap *idmap, struct inode *dir,
+			  struct dentry *dentry, umode_t mode);
 void security_inode_post_create_tmpfile(const struct mnt_idmap *idmap,
 					struct inode *inode);
-int security_inode_link(struct dentry *old_dentry, struct inode *dir,
-			 struct dentry *new_dentry);
+int security_inode_link(const struct mnt_idmap *idmap, struct dentry *old_dentry,
+			struct inode *dir, struct dentry *new_dentry);
 int security_inode_unlink(struct inode *dir, struct dentry *dentry);
-int security_inode_symlink(struct inode *dir, struct dentry *dentry,
-			   const char *old_name);
-int security_inode_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
+int security_inode_symlink(const struct mnt_idmap *idmap, struct inode *dir,
+			   struct dentry *dentry, const char *old_name);
+int security_inode_mkdir(const struct mnt_idmap *idmap, struct inode *dir,
+			 struct dentry *dentry, umode_t mode);
 int security_inode_rmdir(struct inode *dir, struct dentry *dentry);
-int security_inode_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev);
+int security_inode_mknod(const struct mnt_idmap *idmap, struct inode *dir,
+			 struct dentry *dentry, umode_t mode, dev_t dev);
 int security_inode_rename(struct inode *old_dir, struct dentry *old_dentry,
 			  struct inode *new_dir, struct dentry *new_dentry,
 			  unsigned int flags);
 int security_inode_readlink(struct dentry *dentry);
 int security_inode_follow_link(struct dentry *dentry, struct inode *inode,
 			       bool rcu);
-int security_inode_permission(struct inode *inode, int mask);
+int security_inode_permission(const struct mnt_idmap *idmap,
+			      struct inode *inode, int mask);
 int security_inode_setattr(const struct mnt_idmap *idmap,
 			   struct dentry *dentry, struct iattr *attr);
 void security_inode_post_setattr(const struct mnt_idmap *idmap, struct dentry *dentry,
@@ -540,6 +546,9 @@ int security_task_prctl(int option, unsigned long arg2, unsigned long arg3,
 			unsigned long arg4, unsigned long arg5);
 void security_task_to_inode(struct task_struct *p, struct inode *inode);
 int security_create_user_ns(const struct cred *cred);
+int security_namespace_init(struct ns_common *ns);
+void security_namespace_free(struct ns_common *ns);
+int security_namespace_install(const struct nsset *nsset, struct ns_common *ns);
 int security_ipc_permission(struct kern_ipc_perm *ipcp, short flag);
 void security_ipc_getlsmprop(struct kern_ipc_perm *ipcp, struct lsm_prop *prop);
 int security_msg_msg_alloc(struct msg_msg *msg);
@@ -902,9 +911,10 @@ static inline int security_inode_init_security_anon(struct inode *inode,
 	return 0;
 }
 
-static inline int security_inode_create(struct inode *dir,
-					 struct dentry *dentry,
-					 umode_t mode)
+static inline int security_inode_create(const struct mnt_idmap *idmap,
+					struct inode *dir,
+					struct dentry *dentry,
+					umode_t mode)
 {
 	return 0;
 }
@@ -913,9 +923,10 @@ static inline void
 security_inode_post_create_tmpfile(const struct mnt_idmap *idmap, struct inode *inode)
 { }
 
-static inline int security_inode_link(struct dentry *old_dentry,
-				       struct inode *dir,
-				       struct dentry *new_dentry)
+static inline int security_inode_link(const struct mnt_idmap *idmap,
+				      struct dentry *ol
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memset`
- 外部类型：`audit_krule`
- 外部类型：`cred`
- 外部类型：`ctl_table`
- 外部类型：`dentry`
- 外部类型：`dev_t`
- 外部类型：`iattr`
- 外部类型：`inode`
- 外部类型：`kern_ipc_perm`
- 外部类型：`lsm_ctx`
- 外部类型：`lsm_prop_apparmor`
- 外部类型：`lsm_prop_bpf`
- 外部类型：`lsm_prop_selinux`
- 外部类型：`lsm_prop_smack`
- 外部类型：`mnt_idmap`
- 外部类型：`msg_msg`
- 外部类型：`qstr`
- 外部类型：`task_struct`
- 外部类型：`timezone`
- 外部类型：`umode_t`
- 外部类型：`user_namespace`
- 外部类型：`watch`
- 外部类型：`watch_notification`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e29cc8875f` → 本草稿移入 `cases/defect/auto-linux-e29cc8875f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
