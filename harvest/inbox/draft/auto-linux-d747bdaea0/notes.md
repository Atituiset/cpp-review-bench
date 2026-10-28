# auto-linux-d747bdaea0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8a36f53fd5ec3eff01f1b637d42cbdc3821d0493](https://github.com/torvalds/linux/commit/8a36f53fd5ec3eff01f1b637d42cbdc3821d0493) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 275 |
| 编译错误数（gcc syntax-only） | 227（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8a36f53fd5ec3eff01f1b637d42cbdc3821d0493 (https://github.com/torvalds/linux/commit/8a36f53fd5ec3eff01f1b637d42cbdc3821d0493)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1264；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8a36f53fd5ec3eff01f1b637d42cbdc3821d0493 Merge tag 'mm-stable-2026-10-25-19-27' of git://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1212,85 +1212,72 @@ sg_fasync(int fd, struct file *filp, int mode)
 	return fasync_helper(fd, filp, mode, &sfp->async_qp);
 }
 
-static vm_fault_t
-sg_vma_fault(struct vm_fault *vmf)
+static int sg_discontig_init(void *vm_private_data, void **private)
 {
-	struct vm_area_struct *vma = vmf->vma;
-	Sg_fd *sfp;
-	unsigned long offset, len, sa;
-	Sg_scatter_hold *rsv_schp;
-	int k, length;
-
-	if ((NULL == vma) || (!(sfp = (Sg_fd *) vma->vm_private_data)))
-		return VM_FAULT_SIGBUS;
-	rsv_schp = &sfp->reserve;
-	offset = vmf->pgoff << PAGE_SHIFT;
-	if (offset >= rsv_schp->bufflen)
-		return VM_FAULT_SIGBUS;
-	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
-				      "sg_vma_fault: offset=%lu, scatg=%d\n",
-				      offset, rsv_schp->k_use_sg));
-	sa = vma->vm_start;
-	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
-	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
-		len = vma->vm_end - sa;
-		len = (len < length) ? len : length;
-		if (offset < len) {
-			struct page *page = rsv_schp->pages[k] + (offset >> PAGE_SHIFT);
-			get_page(page);	/* increment page count */
-			vmf->page = page;
-			return 0; /* success */
-		}
-		sa += len;
-		offset -= len;
+	const unsigned long req_sz = (unsigned long)*private;
+	Sg_fd *sfp = vm_private_data;
+	Sg_scatter_hold *rsv_schp = &sfp->reserve;
+	int err = 0;
+
+	mutex_lock(&sfp->f_mutex);
+	if (req_sz > rsv_schp->bufflen) {
+		err = -ENOMEM;	/* cannot map more than reserved buffer */
+		goto out;
+	}
+	sfp->mmap_called = 1; /* Prevents changes to buffer size. */
+out:
+	mutex_unlock(&sfp->f_mutex);
+	return err;
+}
+
+static int
+sg_discontig_get(struct discontig_kernel_page_state *state)
+{
+	Sg_fd *sfp = state->vm_private_data;
+	Sg_scatter_hold *rsv_schp = &sfp->reserve;
+	const unsigned int order = rsv_schp->page_order;
+	const pgoff_t nr_pages = state->nr_pages_mapped;
+
+	if (nr_pages >= (rsv_schp->bufflen >> PAGE_SHIFT)) {
+		discontig_kernel_map_abort(state);
+		return 0;
 	}
 
-	return VM_FAULT_SIGBUS;
+	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
+				      "%s: offset=%lu, scatg=%d\n", __func__,
+				      nr_pages << PAGE_SHIFT, rsv_schp->k_use_sg));
+
+	discontig_kernel_map_page(state, rsv_schp->pages[nr_pages >> order]);
+	return 0;
 }
 
-static const struct vm_operations_struct sg_mmap_vm_ops = {
-	.fault = sg_vma_fault,
+static const struct discontig_kernel_page_ops sg_discontig_ops = {
+	.init = sg_discontig_init,
+	.get = sg_discontig_get,
 };
 
 static int
-sg_mmap(struct file *filp, struct vm_area_struct *vma)
+sg_mmap_prepare(struct vm_area_desc *desc)
 {
-	Sg_fd *sfp;
-	unsigned long req_sz, len, sa;
-	Sg_scatter_hold *rsv_schp;
-	int k, length;
-	int ret = 0;
+	Sg_fd *sfp = desc->file->private_data;
+	const unsigned long req_sz = vma_desc_size(desc);
 
-	if ((!filp) || (!vma) || (!(sfp = (Sg_fd *) filp->private_data)))
+	if (!sfp)
 		return -ENXIO;
-	req_sz = vma->vm_end - vma->vm_start;
+
 	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
 				      "sg_mmap starting, vm_start=%p, len=%d\n",
-				      (void *) vma->vm_start, (int) req_sz));
-	if (vma->vm_pgoff)
+				      (void *) desc->start, (int) req_sz));
+
+	if (desc->pgoff)
 		return -EINVAL;	/* want no offset */
-	rsv_schp = &sfp->reserve;
-	mutex_lock(&sfp->f_mutex);
-	if (req_sz > rsv_schp->bufflen) {
-		ret = -ENOMEM;	/* cannot map more than reserved buffer */
-		goto out;
-	}
 
-	sa = vma->vm_start;
-	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
-	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
-		len = vma->vm_end - sa;
-		len = (len < length) ? len : length;
-		sa += len;
-	}
+	vma_desc_set_flags(desc, VMA_DONTEXPAND_BIT, VMA_DONTDUMP_BIT);
+	desc->private_data = sfp;
 
-	sfp->mmap_called = 1;
-	vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
-	vma->vm_private_data = sfp;
-	vma->vm_ops = &sg_mmap_vm_ops;
-out:
-	mutex_unlock(&sfp->f_mutex);
-	return ret;
+	mmap_action_map_discontig_kernel_pages(desc, (void *)req_sz,
+					    
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__free_pages`
- 外部函数：`__module_get`
- 外部函数：`__scsi_print_sense`
- 外部函数：`alloc_pages`
- 外部函数：`atomic_read`
- 外部函数：`bio`
- 外部函数：`blk_execute_rq_nowait`
- 外部函数：`blk_mq_free_request`
- 外部函数：`blk_mq_rq_to_pdu`
- 外部函数：`blk_put_queue`
- 外部函数：`blk_rq_aligned`
- 外部函数：`blk_rq_map_user_io`
- 外部函数：`blk_rq_unmap_user`
- 外部函数：`blk_trace_remove`
- 外部函数：`blk_trace_setup`
- 外部函数：`blk_trace_startstop`
- 外部函数：`container_of`
- 外部函数：`copy_from_user`
- 外部函数：`copy_to_user`
- 外部函数：`driver_byte`
- 外部函数：`fasync_helper`
- 外部函数：`get_order`
- 外部函数：`get_page`
- 外部函数：`get_sg_io_hdr`
- 外部函数：`get_user`
- 外部函数：`host_byte`
- 外部函数：`idr_find`
- 外部函数：`idr_remove`
- 外部函数：`iminor`
- 外部函数：`in_compat_syscall`
- 外部函数：`init_waitqueue_head`
- 外部函数：`ioctl`
- 外部函数：`jiffies_to_msecs`
- 外部函数：`kcalloc`
- 外部函数：`kfree`
- 外部函数：`kill_fasync`
- 外部函数：`kref_get`
- 外部函数：`kref_init`
- 外部函数：`kref_put`
- 外部函数：`kzalloc`
- 外部函数：`kzalloc_obj`
- 外部函数：`likely`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min_t`
- 外部函数：`mmap`
- 外部函数：`module_put`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`mult_frac`
- 外部函数：`mutex_init`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`nonseekable_open`
- 外部函数：`num`
- 外部函数：`of`
- 外部函数：`offsetof`
- 外部函数：`opens`
- 外部函数：`orphan`
- 外部函数：`pr_info`
- 外部函数：`put_sg_io_hdr`
- 外部函数：`put_user`
- 外部函数：`queue_max_sectors`
- 外部函数：`queue_max_segments`
- 外部函数：`read`
- 外部函数：`read_lock_irqsave`
- 外部函数：`read_unlock_irqrestore`
- 外部函数：`release`
- 外部函数：`rwlock_init`
- 外部函数：`schedule_work`
- 外部函数：`scsi_alloc_request`
- 外部函数：`scsi_cmd_allowed`
- 外部函数：`scsi_device_get`
- 外部函数：`scsi_device_put`
- 外部函数：`scsi_ioctl`
- 外部函数：`scsi_ioctl_block_when_processing_errors`
- 外部函数：`scsi_normalize_sense`
- 外部函数：`scsi_sense_is_deferred`
- 外部函数：`sdev_prefix_printk`
- 外部函数：`sg_allow_access`
- 外部函数：`sg_link_reserve`
- 外部函数：`sg_printk`
- 外部函数：`sg_read`
- 外部函数：`sg_status_byte`
- 外部函数：`unlikely`
- 外部函数：`v3`
- 外部函数：`vm_flags_set`
- 外部函数：`wait_event_interruptible`
- 外部函数：`wake_up_interruptible`
- 外部函数：`wake_up_interruptible_all`
- 外部函数：`write`
- 外部函数：`write_lock_irq`
- 外部函数：`write_lock_irqsave`
- 外部函数：`write_unlock_irq`
- 外部函数：`write_unlock_irqrestore`
- 大写宏：`ALIGN`
- 大写宏：`BEFORE`
- 大写宏：`BLKSECTGET`
- 大写宏：`BLKTRACESETUP`
- 大写宏：`BLKTRACESTART`
- 大写宏：`BLKTRACESTOP`
- 大写宏：`BLKTRACETEARDOWN`
- 大写宏：`BLK_MQ_REQ_NOWAIT`
- 大写宏：`CAUTION`
- 大写宏：`CHECK_CONDITION`
- 大写宏：`COMMAND_COMPLETE`
- 大写宏：`COMMAND_TERMINATED`
- 大写宏：`CONFIG_COMPAT`
- 大写宏：`DIRECT_IO`
- 大写宏：`DISK_NAME_LEN`
- 大写宏：`DRIVER_SENSE`
- 大写宏：`EBUSY`
- 大写宏：`EDOM`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`EMSGSIZE`
- 大写宏：`ENODEV`
- 大写宏：`ENOIOCTLCMD`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSYS`
- 大写宏：`ENXIO`
- 大写宏：`EPERM`
- 大写宏：`ERESTARTSYS`
- 大写宏：`ERR_PTR`
- 大写宏：`EWOULDBLOCK`
- 大写宏：`FMODE_WRITE`
- 大写宏：`GFP_ATOMIC`
- 大写宏：`GFP_KERNEL`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`INIT_WORK`
- 大写宏：`INT_MAX`
- 大写宏：`IS_ERR`
- 大写宏：`ITER_DEST`
- 大写宏：`ITER_SOURCE`
- 大写宏：`KERN_INFO`
- 大写宏：`MKDEV`
- 大写宏：`MMAP_IO`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`O_ACCMODE`
- 大写宏：`O_EXCL`
- 大写宏：`O_NDELAY`
- 大写宏：`O_NONBLOCK`
- 大写宏：`O_RDONLY`
- 大写宏：`O_RDWR`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`POLL_IN`
- 大写宏：`PTR_ERR`
- 大写宏：`REQ_OP_DRV_IN`
- 大写宏：`REQ_OP_DRV_OUT`
- 大写宏：`RQ_END_IO_NONE`
- 大写宏：`SCSI`
- 大写宏：`SCSI_GENERIC_MAJOR`
- 大写宏：`SCSI_IOCTL_GET_BUS_NUMBER`
- 大写宏：`SCSI_IOCTL_GET_IDLUN`
- 大写宏：`SCSI_IOCTL_PROBE_HOST`
- 大写宏：`SCSI_IOCTL_SEND_COMMAND`
- 大写宏：`SCSI_LOG_TIMEOUT`
- 大写宏：`SCSI_SENSE_BUFFERSIZE`
- 大写宏：`SG_DEFAULT_RETRIES`
- 大写宏：`SG_DEFAULT_TIMEOUT_USER`
- 大写宏：`SG_DEF_COMMAND_Q`
- 大写宏：`SG_DEF_FORCE_PACK_ID`
- 大写宏：`SG_DEF_KEEP_ORPHAN`
- 大写宏：`SG_DXFER_NONE`
- 大写宏：`SG_DXFER_TO_DEV`
- 大写宏：`SG_DXFER_TO_FROM_DEV`
- 大写宏：`SG_DXFER_UNKNOWN`
- 大写宏：`SG_EMULATED_HOST`
- 大写宏：`SG_FLAG_DIRECT_IO`
- 大写宏：`SG_FLAG_MMAP_IO`
- 大写宏：`SG_FLAG_Q_AT_TAIL`
- 大写宏：`SG_GET_ACCESS_COUNT`
- 大写宏：`SG_GET_COMMAND_Q`
- 大写宏：`SG_GET_KEEP_ORPHAN`
- 大写宏：`SG_GET_LOW_DMA`
- 大写宏：`SG_GET_NUM_WAITING`
- 大写宏：`SG_GET_PACK_ID`
- 大写宏：`SG_GET_REQUEST_TABLE`
- 大写宏：`SG_GET_RESERVED_SIZE`
- 大写宏：`SG_GET_SCSI_ID`
- 大写宏：`SG_GET_SG_TABLESIZE`
- 大写宏：`SG_GET_TIMEOUT`
- 大写宏：`SG_GET_TRANSFORM`
- 大写宏：`SG_GET_VERSION_NUM`
- 大写宏：`SG_INFO_CHECK`
- 大写宏：`SG_INFO_DIRECT_IO`
- 大写宏：`SG_IO`
- 大写宏：`SG_MAX_QUEUE`
- 大写宏：`SG_NEXT_CMD_LEN`
- 大写宏：`SG_SCSI_RESET`
- 大写宏：`SG_SET_COMMAND_Q`
- 大写宏：`SG_SET_DEBUG`
- 大写宏：`SG_SET_FORCE_LOW_DMA`
- 大写宏：`SG_SET_FORCE_PACK_ID`
- 大写宏：`SG_SET_KEEP_ORPHAN`
- 大写宏：`SG_SET_RESERVED_SIZE`
- 大写宏：`SG_SET_TIMEOUT`
- 大写宏：`SIGPOLL`
- 大写宏：`SZ_256M`
- 大写宏：`THIS_MODULE`
- 大写宏：`TYPE_SCANNER`
- 大写宏：`UNIT_ATTENTION`
- 大写宏：`USER_HZ`
- 大写宏：`VM_DONTDUMP`
- 大写宏：`VM_DONTEXPAND`
- 大写宏：`VM_FAULT_SIGBUS`
- 大写宏：`VM_IO`
- 大写宏：`WARN_ON`
- 外部类型：`Additional`
- 外部类型：`Although`
- 外部类型：`But`
- 外部类型：`Called`
- 外部类型：`Can`
- 外部类型：`Cleanup`
- 外部类型：`Count`
- 外部类型：`Detected`
- 外部类型：`Eric`
- 外部类型：`Following`
- 外部类型：`Free`
- 外部类型：`Hence`
- 外部类型：`If`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Permits`
- 外部类型：`Prevent`
- 外部类型：`Rely`
- 外部类型：`Set`
- 外部类型：`Sg_device`
- 外部类型：`Sg_fd`
- 外部类型：`Sg_request`
- 外部类型：`Sg_scatter_hold`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Tread`
- 外部类型：`User`
- 外部类型：`With`
- 外部类型：`Youngdale`
- 外部类型：`atomic_t`
- 外部类型：`bio`
- 外部类型：`blk_status_t`
- 外部类型：`cdev`
- 外部类型：`compat_sg_io_hdr`
- 外部类型：`compat_uptr_t`
- 外部类型：`execute_work`
- 外部类型：`fasync_struct`
- 外部类型：`file`
- 外部类型：`gfp_t`
- 外部类型：`inode`
- 外部类型：`io_comp_batch`
- 外部类型：`kref`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`page`
- 外部类型：`request`
- 外部类型：`request_queue`
- 外部类型：`rq_end_io_ret`
- 外部类型：`rq_map_data`
- 外部类型：`rwlock_t`
- 外部类型：`scsi_cmnd`
- 外部类型：`scsi_device`
- 外部类型：`scsi_sense_hdr`
- 外部类型：`sg_io_hdr_t`
- 外部类型：`sg_req_info`
- 外部类型：`sg_req_info_t`
- 外部类型：`sg_scsi_id_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`uintptr_t`
- 外部类型：`vm_area_struct`
- 外部类型：`vm_fault`
- 外部类型：`vm_fault_t`
- 外部类型：`vm_operations_struct`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-d747bdaea0` → 本草稿移入 `cases/defect/auto-linux-d747bdaea0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
