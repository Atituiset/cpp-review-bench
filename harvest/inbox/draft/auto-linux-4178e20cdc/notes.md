# auto-linux-4178e20cdc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2a49d49159b76f6dd587131b3765d3458cc3aa01](https://github.com/torvalds/linux/commit/2a49d49159b76f6dd587131b3765d3458cc3aa01) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 118 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2a49d49159b76f6dd587131b3765d3458cc3aa01 (https://github.com/torvalds/linux/commit/2a49d49159b76f6dd587131b3765d3458cc3aa01)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 409；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2a49d49159b76f6dd587131b3765d3458cc3aa01 Merge tag 'vfs-7.4-rc1.netfs' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -19,7 +19,7 @@
 struct cachefiles_kiocb {
 	struct kiocb		iocb;
 	refcount_t		ki_refcnt;
-	loff_t			start;
+	uoff_t			start;
 	union {
 		size_t		skipped;
 		size_t		len;
@@ -32,6 +32,8 @@ struct cachefiles_kiocb {
 	u64			b_writing;
 };
 
+#define IS_ERR_VALUE_LL(x) unlikely((x) >= (unsigned long long)-MAX_ERRNO)
+
 static inline void cachefiles_put_kiocb(struct cachefiles_kiocb *ki)
 {
 	if (refcount_dec_and_test(&ki->ki_refcnt)) {
@@ -73,7 +75,7 @@ static void cachefiles_read_complete(struct kiocb *iocb, long ret)
  * Initiate a read from the cache.
  */
 static int cachefiles_read(struct netfs_cache_resources *cres,
-			   loff_t start_pos,
+			   uoff_t start_pos,
 			   struct iov_iter *iter,
 			   enum netfs_read_from_hole read_hole,
 			   netfs_io_terminated_t term_func,
@@ -193,60 +195,81 @@ static int cachefiles_read(struct netfs_cache_resources *cres,
 }
 
 /*
- * Query the occupancy of the cache in a region, returning where the next chunk
- * of data starts and how long it is.
+ * Query the occupancy of the cache in a region, returning the extent of the
+ * next two chunks of cached data and the next hole.
  */
 static int cachefiles_query_occupancy(struct netfs_cache_resources *cres,
-				      loff_t start, size_t len, size_t granularity,
-				      loff_t *_data_start, size_t *_data_len)
+				      struct fscache_occupancy *occ)
 {
 	struct cachefiles_object *object;
+	struct inode *inode;
 	struct file *file;
-	loff_t off, off2;
-
-	*_data_start = -1;
-	*_data_len = 0;
+	uoff_t read_limit;
+	loff_t ret;
+	int i;
 
 	if (!fscache_wait_for_operation(cres, FSCACHE_WANT_READ))
 		return -ENOBUFS;
 
 	object = cachefiles_cres_object(cres);
 	file = cachefiles_cres_file(cres);
-	granularity = max_t(size_t, object->volume->cache->bsize, granularity);
+	inode = file_inode(file);
+	occ->granularity = object->volume->cache->bsize;
+	/* Read read_limit before content_info. */
+	read_limit = atomic64_read_acquire(&object->read_limit);
+
+	_enter("%pD,%llu,%llx-%llx/%llx",
+	       file, inode->i_ino, occ->query_from, occ->query_to, read_limit);
+
+	if (read_limit == 0)
+		goto done;
+
+	switch (READ_ONCE(object->content_info)) {
+	case CACHEFILES_CONTENT_ALL:
+	case CACHEFILES_CONTENT_SINGLE:
+		if (read_limit > occ->query_from) {
+			occ->cached_from[0] = 0;
+			occ->cached_to[0] = read_limit;
+			occ->cached_type[0] = FSCACHE_EXTENT_DATA;
+			occ->query_from = ULLONG_MAX;
+		}
+		goto done;
+	default:
+		break;
+	}
 
-	_enter("%pD,%llu,%llx,%zx/%llx",
-	       file, file_inode(file)->i_ino, start, len,
-	       i_size_read(file_inode(file)));
+	for (i = 0; i < ARRAY_SIZE(occ->cached_from); i++) {
+		ret = cachefiles_inject_read_error();
+		if (ret == 0)
+			ret = vfs_llseek(file, occ->query_from, SEEK_DATA);
+		if (IS_ERR_VALUE_LL(ret)) {
+			if (ret != -ENXIO)
+				return ret;
+			occ->query_from = ULLONG_MAX;
+			goto done;
+		}
+		occ->cached_type[i] = FSCACHE_EXTENT_DATA;
+		occ->cached_from[i] = ret;
+		occ->query_from = ret;
+
+		ret = cachefiles_inject_read_error();
+		if (ret == 0)
+			ret = vfs_llseek(file, occ->query_from, SEEK_HOLE);
+		if (IS_ERR_VALUE_LL(ret)) {
+			if (ret != -ENXIO)
+				return ret;
+			occ->query_from = ULLONG_MAX;
+			goto done;
+		}
+		occ->cached_to[i] = ret;
+		occ->query_from = ret;
+		if (occ->query_from >= occ->query_to)
+			break;
+	}
 
-	off = cachefiles_inject_read_error();
-	if (off == 0)
-		off = vfs_llseek(file, start, SEEK_DATA);
-	if (off == -ENXIO)
-		return -ENODATA; /* Beyond EOF */
-	if (off < 0 && off >= (loff_t)-MAX_ERRNO)
-		return -ENOBUFS; /* Error. */
-	if (round_up(off, granularity) >= start + len)
-		return -ENODATA; /* No data in range */
-
-	off2 = cachefiles_inject_read_error();
-	if (off2 == 0)
-		off2 = vfs_llseek(file, off, SEEK_HOLE);
-	if (off2 == -ENXIO)
-		return -ENODATA; /* Beyond EOF */
-	if (off2 < 0 && off2 >= (loff_t)-MAX_ERRNO)
-		return -ENOBUFS; /* Error. */
-
-	/* Round away partial blocks */
-	off = round_up(off, granularity);
-	off2 = 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__set_bit`
- 外部函数：`_enter`
- 外部函数：`_leave`
- 外部函数：`atomic_long_add`
- 外部函数：`atomic_long_sub`
- 外部函数：`cachefiles_begin_secure`
- 外部函数：`cachefiles_cres_file`
- 外部函数：`cachefiles_cres_object`
- 外部函数：`cachefiles_end_secure`
- 外部函数：`cachefiles_grab_object`
- 外部函数：`cachefiles_has_space`
- 外部函数：`cachefiles_inject_read_error`
- 外部函数：`cachefiles_inject_write_error`
- 外部函数：`container_of`
- 外部函数：`file_inode`
- 外部函数：`fscache_count_dio_misfit`
- 外部函数：`fscache_count_read`
- 外部函数：`fscache_count_write`
- 外部函数：`fscache_cres_cookie`
- 外部函数：`fscache_wait_for_operation`
- 外部函数：`get_current_ioprio`
- 外部函数：`get_file`
- 外部函数：`i_size_read`
- 外部函数：`iov_iter_advance`
- 外部函数：`iov_iter_count`
- 外部函数：`iov_iter_truncate`
- 外部函数：`iov_iter_zero`
- 外部函数：`kiocb_end_write`
- 外部函数：`kzalloc_obj`
- 外部函数：`max_t`
- 外部函数：`memalloc_nofs_restore`
- 外部函数：`memalloc_nofs_save`
- 外部函数：`netfs_prepare_write_failed`
- 外部函数：`netfs_write_subrequest_terminated`
- 外部函数：`refcount_dec_and_test`
- 外部函数：`refcount_set`
- 外部函数：`round_down`
- 外部函数：`round_up`
- 外部函数：`set_bit`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`term_func`
- 外部函数：`test_bit`
- 外部函数：`trace_cachefiles_io_error`
- 外部函数：`trace_cachefiles_prep_read`
- 外部函数：`trace_cachefiles_read`
- 外部函数：`trace_cachefiles_write`
- 外部函数：`trace_netfs_sreq`
- 外部函数：`vfs_iocb_iter_read`
- 外部函数：`vfs_iocb_iter_write`
- 外部函数：`vfs_llseek`
- 大写宏：`AIO`
- 大写宏：`BIO_MAX_VECS`
- 大写宏：`CACHEFILES_DIO_BLOCK_SIZE`
- 大写宏：`DIO`
- 大写宏：`EINTR`
- 大写宏：`EIOCBQUEUED`
- 大写宏：`ENOBUFS`
- 大写宏：`ENODATA`
- 大写宏：`ENOMEM`
- 大写宏：`ENXIO`
- 大写宏：`EOF`
- 大写宏：`ERESTARTNOHAND`
- 大写宏：`ERESTARTNOINTR`
- 大写宏：`ERESTARTSYS`
- 大写宏：`ERESTART_RESTARTBLOCK`
- 大写宏：`ESTALE`
- 大写宏：`FSCACHE_COOKIE_HAVE_DATA`
- 大写宏：`FSCACHE_COOKIE_NO_DATA_TO_READ`
- 大写宏：`FSCACHE_WANT_READ`
- 大写宏：`FSCACHE_WANT_WRITE`
- 大写宏：`IOCB_DIRECT`
- 大写宏：`IOCB_WRITE`
- 大写宏：`MAX_ERRNO`
- 大写宏：`MAX_RW_COUNT`
- 大写宏：`NETFS_DOWNLOAD_FROM_SERVER`
- 大写宏：`NETFS_FILL_WITH_ZEROES`
- 大写宏：`NETFS_READ_FROM_CACHE`
- 大写宏：`NETFS_READ_HOLE_FAIL`
- 大写宏：`NETFS_READ_HOLE_IGNORE`
- 大写宏：`NETFS_SREQ_COPY_TO_CACHE`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`SEEK_DATA`
- 大写宏：`SEEK_HOLE`
- 外部类型：`Beyond`
- 外部类型：`Clean`
- 外部类型：`Enough`
- 外部类型：`Error`
- 外部类型：`If`
- 外部类型：`Initiate`
- 外部类型：`Just`
- 外部类型：`No`
- 外部类型：`Prepare`
- 外部类型：`Probably`
- 外部类型：`Query`
- 外部类型：`Round`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`We`
- 外部类型：`cachefiles_cache`
- 外部类型：`cachefiles_object`
- 外部类型：`cachefiles_prepare_read_trace`
- 外部类型：`cred`
- 外部类型：`file`
- 外部类型：`fscache_cookie`
- 外部类型：`fscache_want_state`
- 外部类型：`ino_t`
- 外部类型：`inode`
- 外部类型：`iov_iter`
- 外部类型：`kiocb`
- 外部类型：`loff_t`
- 外部类型：`netfs_cache_resources`
- 外部类型：`netfs_io_request`
- 外部类型：`netfs_io_source`
- 外部类型：`netfs_io_stream`
- 外部类型：`netfs_io_subrequest`
- 外部类型：`netfs_io_terminated_t`
- 外部类型：`netfs_read_from_hole`
- 外部类型：`refcount_t`
- 外部类型：`size_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-4178e20cdc` → 本草稿移入 `cases/defect/auto-linux-4178e20cdc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
