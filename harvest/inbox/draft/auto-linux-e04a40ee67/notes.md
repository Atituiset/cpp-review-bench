# auto-linux-e04a40ee67

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ff47652a4b66c067c765a7ad464d930b5a9367cc](https://github.com/torvalds/linux/commit/ff47652a4b66c067c765a7ad464d930b5a9367cc) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 127 |
| 编译错误数（gcc syntax-only） | 82（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ff47652a4b66c067c765a7ad464d930b5a9367cc (https://github.com/torvalds/linux/commit/ff47652a4b66c067c765a7ad464d930b5a9367cc)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ff47652a4b66c067c765a7ad464d930b5a9367cc Merge tag 'cifs-fixes-7.3-rc6' of https://git.manguebit.org/linux :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -469,6 +469,7 @@ ssize_t netfs_buffered_write_iter_locked(struct kiocb *iocb, struct iov_iter *fr
 					 struct netfs_group *netfs_group)
 {
 	struct file *file = iocb->ki_filp;
+	struct inode *inode = file_inode(file);
 	ssize_t ret;
 
 	trace_netfs_write_iter(iocb, from);
@@ -481,6 +482,14 @@ ssize_t netfs_buffered_write_iter_locked(struct kiocb *iocb, struct iov_iter *fr
 	if (ret)
 		return ret;
 
+	if (iocb->ki_pos > i_size_read(inode)) {
+		ret = netfs_clear_stale_pre_isize(inode, i_size_read(inode),
+						  iocb->ki_pos,
+						  iocb->ki_flags & IOCB_NOWAIT);
+		if (ret)
+			return ret;
+	}
+
 	return netfs_perform_write(iocb, from, netfs_group);
 }
 EXPORT_SYMBOL(netfs_buffered_write_iter_locked);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__filemap_get_folio`
- 外部函数：`copy_folio_from_iter_atomic`
- 外部函数：`fault_in_iov_iter_readable`
- 外部函数：`fgf_set_order`
- 外部函数：`file_inode`
- 外部函数：`filemap_write_and_wait_range`
- 外部函数：`flush_dcache_folio`
- 外部函数：`folio_get_private`
- 外部函数：`folio_pos`
- 外部函数：`folio_size`
- 外部函数：`folio_test_uptodate`
- 外部函数：`folio_wait_writeback_killable`
- 外部函数：`folio_zero_segment`
- 外部函数：`fscache_update_cookie`
- 外部函数：`i_size_read`
- 外部函数：`i_size_write`
- 外部函数：`iov_iter_count`
- 外部函数：`is_sync_kiocb`
- 外部函数：`mapping_gfp_mask`
- 外部函数：`mapping_large_folio_support`
- 外部函数：`mapping_max_folio_size`
- 外部函数：`mapping_writably_mapped`
- 外部函数：`min`
- 外部函数：`min_t`
- 外部函数：`netfs_begin_writethrough`
- 外部函数：`netfs_folio_group`
- 外部函数：`netfs_folio_info`
- 外部函数：`netfs_inode`
- 外部函数：`netfs_read_zero_point`
- 外部函数：`netfs_stat`
- 外部函数：`signal_pending`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`unlikely`
- 外部函数：`update_i_size`
- 外部函数：`wbc_attach_fdatawrite_inode`
- 外部函数：`wbc_detach_inode`
- 大写宏：`BDP_ASYNC`
- 大写宏：`CONFIG_FSCACHE`
- 大写宏：`COPY_TO_CACHE`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`EFAULT`
- 大写宏：`EINTR`
- 大写宏：`ERESTARTSYS`
- 大写宏：`FGP_WRITEBEGIN`
- 大写宏：`IOCB_DSYNC`
- 大写宏：`IOCB_NOWAIT`
- 大写宏：`IOCB_SYNC`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`LONG_MAX`
- 大写宏：`LRU`
- 大写宏：`NETFS_FOLIO_COPY_TO_CACHE`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PTR_ERR`
- 大写宏：`RMW`
- 大写宏：`SECTOR_SIZE`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WB_SYNC_NONE`
- 外部类型：`Bring`
- 外部类型：`Bytes`
- 外部类型：`Decide`
- 外部类型：`If`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Offset`
- 外部类型：`The`
- 外部类型：`Wait`
- 外部类型：`We`
- 外部类型：`address_space`
- 外部类型：`blkcnt_t`
- 外部类型：`fgf_t`
- 外部类型：`file`
- 外部类型：`folio`
- 外部类型：`inode`
- 外部类型：`iov_iter`
- 外部类型：`kiocb`
- 外部类型：`loff_t`
- 外部类型：`netfs_folio`
- 外部类型：`netfs_folio_trace`
- 外部类型：`netfs_group`
- 外部类型：`netfs_inode`
- 外部类型：`netfs_io_request`
- 外部类型：`pgoff_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
- 外部类型：`writeback_control`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e04a40ee67` → 本草稿移入 `cases/defect/auto-linux-e04a40ee67/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
