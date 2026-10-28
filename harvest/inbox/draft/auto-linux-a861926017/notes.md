# auto-linux-a861926017

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#03973f7f3556b769a3dd5104f166614d446351c9](https://github.com/torvalds/linux/commit/03973f7f3556b769a3dd5104f166614d446351c9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 173 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #03973f7f3556b769a3dd5104f166614d446351c9 (https://github.com/torvalds/linux/commit/03973f7f3556b769a3dd5104f166614d446351c9)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1228；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 03973f7f3556b769a3dd5104f166614d446351c9 Merge tag 'fuse-update-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/mszeredi/fuse :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -11,6 +11,8 @@
 # define pr_fmt(fmt) "fuse: " fmt
 #endif
 
+#define fuse_EIO(fmt, ...) (pr_notice_once("%s: " fmt "\n", __func__, ##__VA_ARGS__), -EIO)
+
 #include "args.h"
 #include <linux/fuse.h>
 #include <linux/fs.h>
@@ -22,14 +24,15 @@
 #include <linux/backing-dev.h>
 #include <linux/mutex.h>
 #include <linux/rwsem.h>
-#include <linux/rbtree.h>
+#include <linux/rbtree_types.h>
 #include <linux/poll.h>
 #include <linux/workqueue.h>
 #include <linux/kref.h>
 #include <linux/xattr.h>
 #include <linux/pid_namespace.h>
 #include <linux/refcount.h>
 #include <linux/user_namespace.h>
+#include <linux/rhashtable-types.h>
 
 /** Default max number of pages that can be used in a single read request */
 #define FUSE_DEFAULT_MAX_PAGES_PER_REQ 32
@@ -86,11 +89,36 @@ struct fuse_submount_lookup {
 	struct fuse_forget_link *forget;
 };
 
+enum fuse_backing_type {
+	FUSE_BACKING_PATH,
+	FUSE_BACKING_DAXDEV,
+	FUSE_BACKING_EXTMAP,
+};
+
+struct fuse_iext;
+
 /* Container for data related to mapping to backing file */
 struct fuse_backing {
-	struct file *file;
-	const struct cred *cred;
+	enum fuse_backing_type type;
 
+	union {
+		struct {
+			struct path path;
+			const struct cred *cred;
+		};
+		struct {
+			struct dax_device *dax_dev;
+			bool dax_error;
+		};
+		struct {
+			struct fuse_iext *extents;
+			unsigned int nr_extents;
+			u64 ext_len;
+			u64 cycle_length;
+		};
+	};
+	u64 backing_id;
+	struct rhash_head hash_node;
 	/* refcount */
 	refcount_t count;
 	struct rcu_head rcu;
@@ -218,11 +246,11 @@ struct fuse_inode {
 	/** @lock: Lock to protect write-related fields */
 	spinlock_t lock;
 
-#ifdef CONFIG_FUSE_DAX
+#ifdef CONFIG_FUSE_VDAX
 	/**
-	 * @dax: Dax specific inode data
+	 * @vdax: Virtiofs DAX specific inode data
 	 */
-	struct fuse_inode_dax *dax;
+	struct fuse_inode_vdax *vdax;
 #endif
 	/** @submount_lookup: Submount specific lookup tracking */
 	struct fuse_submount_lookup *submount_lookup;
@@ -364,16 +392,16 @@ struct fuse_io_priv {
 	.iocb = i,			\
 }
 
-enum fuse_dax_mode {
-	FUSE_DAX_INODE_DEFAULT,	/* default */
-	FUSE_DAX_ALWAYS,	/* "-o dax=always" */
-	FUSE_DAX_NEVER,		/* "-o dax=never" */
-	FUSE_DAX_INODE_USER,	/* "-o dax=inode" */
+enum fuse_vdax_mode {
+	FUSE_VDAX_INODE_DEFAULT,	/* default */
+	FUSE_VDAX_ALWAYS,		/* "-o dax=always" */
+	FUSE_VDAX_NEVER,		/* "-o dax=never" */
+	FUSE_VDAX_INODE_USER,		/* "-o dax=inode" */
 };
 
-static inline bool fuse_is_inode_dax_mode(enum fuse_dax_mode mode)
+static inline bool fuse_is_inode_vdax_mode(enum fuse_vdax_mode mode)
 {
-	return mode == FUSE_DAX_INODE_DEFAULT || mode == FUSE_DAX_INODE_USER;
+	return mode == FUSE_VDAX_INODE_DEFAULT || mode == FUSE_VDAX_INODE_USER;
 }
 
 struct fuse_fs_context {
@@ -391,13 +419,14 @@ struct fuse_fs_context {
 	bool no_control:1;
 	bool no_force_umount:1;
 	bool legacy_opts_show:1;
-	enum fuse_dax_mode dax_mode;
+	bool syncfs_capable:1;
+	enum fuse_vdax_mode vdax_mode;
 	unsigned int max_read;
 	unsigned int blksize;
 	const char *subtype;
 
-	/* DAX device, may be NULL */
-	struct dax_device *dax_dev;
+	/* Virtiofs DAX device, may be NULL */
+	struct dax_device *vdax_dev;
 };
 
 struct fuse_sync_bucket {
@@ -675,6 +704,14 @@ struct fuse_conn {
 	/** @sync_fs: Propagate syncfs() to server */
 	unsigned int sync_fs:1;
 
+	/**
+	 * @syncfs_capable: the privilege required to honor FUSE_HAS_SYNCFS was
+	 * present when /dev/fuse was opened (CAP_SYS_ADMIN in the initial user
+	 * namespace), i.e. the same privilege that mounting virtiofs/fuseblk
+	 * requires.
+	 */
+	unsigned int syncfs_capable:1;
+
 	/** @init_security: Initialize security xattrs when creating a new inode */
 	unsigned int init_security:1;
 
@@ -684,8 +721,8 @@ struct fuse_conn {
 	 */
 	unsigned int create_supp_group:1;
 
-	/** @inode_dax: Does the filesystem support per inode DAX? */
-	unsigned int inode_dax:1;
+	/** @inode_vdax: Does the filesystem support per inode virtiofs DAX? */
+	unsigned int inode_vdax:1;
 
 	/** @no_tmpfile: Is tmpfile not implement
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cache`
- 外部函数：`connection`
- 外部函数：`failed`
- 外部函数：`fuse_backing_files_free`
- 外部函数：`fuse_backing_files_init`
- 外部函数：`fuse_dax_break_layouts`
- 外部函数：`fuse_dax_cancel_work`
- 外部函数：`fuse_dax_check_alignment`
- 外部函数：`fuse_dax_conn_alloc`
- 外部函数：`fuse_dax_conn_free`
- 外部函数：`fuse_dax_dontcache`
- 外部函数：`fuse_dax_inode_alloc`
- 外部函数：`fuse_dax_inode_cleanup`
- 外部函数：`fuse_dax_inode_init`
- 外部函数：`fuse_dax_mmap`
- 外部函数：`fuse_dax_read_iter`
- 外部函数：`fuse_dax_write_iter`
- 外部函数：`fuse_direct_io`
- 外部函数：`fuse_do_ioctl`
- 外部函数：`fuse_file_cached_io_open`
- 外部函数：`fuse_file_ioctl`
- 外部函数：`fuse_inode_uncached_io_end`
- 外部函数：`fuse_inode_uncached_io_start`
- 外部函数：`fuse_ioctl_common`
- 外部函数：`fuse_passthrough_mmap`
- 外部函数：`fuse_passthrough_open`
- 外部函数：`fuse_passthrough_read_iter`
- 外部函数：`fuse_passthrough_release`
- 外部函数：`fuse_sysctl_register`
- 外部函数：`fuse_sysctl_unregister`
- 外部函数：`open`
- 外部函数：`policy`
- 外部函数：`pr_fmt`
- 外部函数：`readdir`
- 外部函数：`release`
- 外部函数：`syncfs`
- 外部函数：`void`
- 大写宏：`BSD`
- 大写宏：`CAP_FSETID`
- 大写宏：`CONFIG_FUSE_DAX`
- 大写宏：`CONFIG_FUSE_PASSTHROUGH`
- 大写宏：`CONFIG_SYSCTL`
- 大写宏：`CUSE`
- 大写宏：`DAX`
- 大写宏：`DESTROY`
- 大写宏：`FOPEN_`
- 大写宏：`FOPEN_DIRECT_IO`
- 大写宏：`FORGET`
- 大写宏：`FUSE`
- 大写宏：`FUSE_DAX_ALWAYS`
- 大写宏：`FUSE_DAX_INODE_DEFAULT`
- 大写宏：`FUSE_DAX_INODE_USER`
- 大写宏：`FUSE_DAX_NEVER`
- 大写宏：`FUSE_INIT`
- 大写宏：`FUSE_NOWRITE`
- 大写宏：`IDR`
- 大写宏：`INIT`
- 大写宏：`IOM_CACHED`
- 大写宏：`IOM_NONE`
- 大写宏：`IOM_UNCACHED`
- 大写宏：`IS_DAX`
- 大写宏：`IS_ENABLED`
- 大写宏：`MNT_FORCE`
- 大写宏：`NFS`
- 大写宏：`NULL`
- 大写宏：`O_TRUNC`
- 大写宏：`READ`
- 大写宏：`READLINK`
- 大写宏：`SETATTR`
- 大写宏：`WRITE`
- 外部类型：`Add`
- 外部类型：`Allow`
- 外部类型：`Are`
- 外部类型：`Argument`
- 外部类型：`Auto`
- 外部类型：`Cache`
- 外部类型：`Called`
- 外部类型：`Cannot`
- 外部类型：`Check`
- 外部类型：`Connection`
- 外部类型：`Constrain`
- 外部类型：`Container`
- 外部类型：`Current`
- 外部类型：`Dax`
- 外部类型：`Default`
- 外部类型：`Delete`
- 外部类型：`Device`
- 外部类型：`Dir`
- 外部类型：`Do`
- 外部类型：`Does`
- 外部类型：`Don`
- 外部类型：`Entry`
- 外部类型：`File`
- 外部类型：`Files`
- 外部类型：`Filesystem`
- 外部类型：`Fuse`
- 外部类型：`Has`
- 外部类型：`If`
- 外部类型：`Initialize`
- 外部类型：`Inode`
- 外部类型：`Is`
- 外部类型：`Kernel`
- 外部类型：`Key`
- 外部类型：`List`
- 外部类型：`Lock`
- 外部类型：`Maximum`
- 外部类型：`Miscellaneous`
- 外部类型：`Negotiated`
- 外部类型：`New`
- 外部类型：`Node`
- 外部类型：`Number`
- 外部类型：`Offset`
- 外部类型：`Only`
- 外部类型：`Passthrough`
- 外部类型：`Propagate`
- 外部类型：`Protected`
- 外部类型：`Read`
- 外部类型：`Readdir`
- 外部类型：`Refcount`
- 外部类型：`Reference`
- 外部类型：`Relax`
- 外部类型：`Return`
- 外部类型：`Send`
- 外部类型：`Submount`
- 外部类型：`Super`
- 外部类型：`The`
- 外部类型：`Time`
- 外部类型：`Unique`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Version`
- 外部类型：`Wait`
- 外部类型：`Waitq`
- 外部类型：`Which`
- 外部类型：`Writepages`
- 外部类型：`atomic64_t`
- 外部类型：`atomic_t`
- 外部类型：`completion`
- 外部类型：`cred`
- 外部类型：`dax_device`
- 外部类型：`dentry`
- 外部类型：`dev_t`
- 外部类型：`file`
- 外部类型：`file_kattr`
- 外部类型：`fuse_args`
- 外部类型：`fuse_chan`
- 外部类型：`fuse_conn_dax`
- 外部类型：`fuse_forget_link`
- 外部类型：`fuse_inode_dax`
- 外部类型：`fuse_open_out`
- 外部类型：`fuse_release_in`
- 外部类型：`idr`
- 外部类型：`inode`
- 外部类型：`iov_iter`
- 外部类型：`kgid_t`
- 外部类型：`kiocb`
- 外部类型：`kref`
- 外部类型：`kuid_t`
- 外部类型：`list_head`
- 外部类型：`loff_t`
- 外部类型：`mutex`
- 外部类型：`pid_namespace`
- 外部类型：`rb_node`
- 外部类型：`rb_root`
- 外部类型：`rcu_head`
- 外部类型：`refcount_t`
- 外部类型：`rw_semaphore`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`ssize_t`
- 外部类型：`super_block`
- 外部类型：`timespec64`
- 外部类型：`umode_t`
- 外部类型：`user_namespace`
- 外部类型：`vm_area_struct`
- 外部类型：`wait_queue_head_t`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a861926017` → 本草稿移入 `cases/defect/auto-linux-a861926017/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
