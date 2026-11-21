# auto-linux-3096efb74e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5f22fb20497f4983094131f00a734362300d22d2](https://github.com/torvalds/linux/commit/5f22fb20497f4983094131f00a734362300d22d2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 53 |
| 编译错误数（gcc syntax-only） | 59（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5f22fb20497f4983094131f00a734362300d22d2 (https://github.com/torvalds/linux/commit/5f22fb20497f4983094131f00a734362300d22d2)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 492；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5f22fb20497f4983094131f00a734362300d22d2 Merge tag 'fuse-fixes-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/mszeredi/fuse :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -193,6 +193,7 @@ static void virtio_fs_ktype_release(struct kobject *kobj)
 {
 	struct virtio_fs *vfs = container_of(kobj, struct virtio_fs, kobj);
 
+	kfree(vfs->tag);
 	kfree(vfs->mq_map);
 	kfree(vfs->vqs);
 	kfree(vfs);
@@ -489,7 +490,11 @@ static void virtio_fs_free_devs(struct virtio_fs *fs)
 	}
 }
 
-/* Read filesystem name from virtio config into fs->tag (must kfree()). */
+/*
+ * Read filesystem name from virtio config into fs->tag.  Freed by
+ * virtio_fs_ktype_release() so that it stays valid for as long as the
+ * virtio_fs object itself, which may outlive the virtio device.
+ */
 static int virtio_fs_read_tag(struct virtio_device *vdev, struct virtio_fs *fs)
 {
 	char tag_buf[sizeof_field(struct virtio_fs_config, tag)];
@@ -505,7 +510,7 @@ static int virtio_fs_read_tag(struct virtio_device *vdev, struct virtio_fs *fs)
 		end = &tag_buf[sizeof(tag_buf)];
 
 	len = end - tag_buf;
-	fs->tag = devm_kmalloc(&vdev->dev, len + 1, GFP_KERNEL);
+	fs->tag = kmalloc(len + 1, GFP_KERNEL);
 	if (!fs->tag)
 		return -ENOMEM;
 	memcpy(fs->tag, tag_buf, len);
@@ -696,15 +701,16 @@ static int copy_args_to_argbuf(struct fuse_req *req, gfp_t gfp)
 	unsigned int offset = 0;
 	unsigned int num_in;
 	unsigned int num_out;
-	unsigned int len;
+	unsigned int in_len;
+	unsigned int out_len;
 	unsigned int i;
 
 	num_in = args->in_numargs - args->in_pages;
 	num_out = args->out_numargs - args->out_pages;
-	len = fuse_len_args(num_in, (struct fuse_arg *) args->in_args) +
-	      fuse_len_args(num_out, args->out_args);
+	in_len = fuse_len_args(num_in, (struct fuse_arg *) args->in_args);
+	out_len = fuse_len_args(num_out, args->out_args);
 
-	req->argbuf = kmalloc(len, gfp);
+	req->argbuf = kmalloc(in_len + out_len, gfp);
 	if (!req->argbuf)
 		return -ENOMEM;
 
@@ -715,6 +721,14 @@ static int copy_args_to_argbuf(struct fuse_req *req, gfp_t gfp)
 		offset += args->in_args[i].size;
 	}
 
+	/*
+	 * The server is not required to fill the reply region and the length
+	 * it reports in the used ring cannot confirm that it did.  Zero the
+	 * region so that an unwritten reply yields zeroes rather than stale
+	 * heap contents.
+	 */
+	memset(req->argbuf + in_len, 0, out_len);
+
 	return 0;
 }
 
@@ -1588,6 +1602,13 @@ static inline void virtio_fs_ctx_set_defaults(struct fuse_fs_context *ctx)
 	ctx->destroy = true;
 	ctx->no_control = true;
 	ctx->no_force_umount = true;
+	/*
+	 * A request handed to the device stays referenced by the virtqueue
+	 * until the device completes it.  The request timeout works by
+	 * aborting the connection, which ends and frees such requests behind
+	 * the device's back, so it must not be enabled here.
+	 */
+	ctx->no_req_timeout = true;
 }
 
 static int virtio_fs_fill_super(struct super_block *sb, struct fs_context *fsc)
@@ -1807,6 +1828,7 @@ static int virtio_fs_init_fs_context(struct fs_context *fsc)
 	ctx = kzalloc_obj(struct fuse_fs_context);
 	if (!ctx)
 		return -ENOMEM;
+	ctx->is_virtiofs = true;
 	fsc->fs_private = ctx;
 	fsc->ops = &virtio_fs_context_ops;
 	return 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`devm_kmalloc`
- 外部函数：`fuse_dev_alloc`
- 外部函数：`fuse_dev_install`
- 外部函数：`fuse_dev_put`
- 外部函数：`fuse_fill_super_common`
- 外部函数：`fuse_len_args`
- 外部函数：`fuse_send_init`
- 外部函数：`get_fuse_mount_super`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_empty`
- 外部函数：`memcpy`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`sizeof_field`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`virtio_fs_read_tag`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`FUSE_VDAX_ALWAYS`
- 大写宏：`FUSE_VDAX_NEVER`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`S_IFDIR`
- 大写宏：`UINT_MAX`
- 外部类型：`After`
- 外部类型：`Allocate`
- 外部类型：`End`
- 外部类型：`In`
- 外部类型：`No`
- 外部类型：`Previous`
- 外部类型：`Read`
- 外部类型：`Start`
- 外部类型：`Though`
- 外部类型：`completion`
- 外部类型：`fs_context`
- 外部类型：`fuse_arg`
- 外部类型：`fuse_conn`
- 外部类型：`fuse_dev`
- 外部类型：`fuse_fs_context`
- 外部类型：`fuse_mount`
- 外部类型：`kobject`
- 外部类型：`list_head`
- 外部类型：`spinlock_t`
- 外部类型：`super_block`
- 外部类型：`virtio_device`
- 外部类型：`virtio_fs`
- 外部类型：`virtio_fs_config`
- 外部类型：`virtqueue`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-3096efb74e` → 本草稿移入 `cases/defect/auto-linux-3096efb74e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
