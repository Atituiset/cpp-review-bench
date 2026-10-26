# auto-linux-f89773aeb0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3](https://github.com/torvalds/linux/commit/9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 58 |
| 编译错误数（gcc syntax-only） | 36（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 (https://github.com/torvalds/linux/commit/9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 696；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 Merge tag 'nfs-for-7.4-1' of git://git.linux-nfs.org/projects/anna/linux-nfs :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -57,7 +57,7 @@ struct nfs4_pnfs_ds_addr {
 };
 
 struct nfs4_pnfs_ds {
-	struct list_head	ds_node;  /* nfs4_pnfs_dev_hlist dev_dslist */
+	struct hlist_node	ds_node;  /* nfs_net nfs4_data_server_cache */
 	char			*ds_remotestr;	/* comma sep list of addrs */
 	struct list_head	ds_addrs;
 	const struct net	*ds_net;
@@ -171,14 +171,34 @@ struct pnfs_layoutdriver_type {
 	struct nfs4_deviceid_node * (*alloc_deviceid_node)
 			(struct nfs_server *server, struct pnfs_device *pdev,
 			gfp_t gfp_flags);
+	/*
+	 * Re-resolve @lo's references to the changed deviceid @id.  Called
+	 * under @lo's inode i_lock inside an RCU read-side critical section:
+	 * must not sleep, allocations are GFP_ATOMIC.  Rather than put the
+	 * references it gives up (the final put can sleep), the hook
+	 * allocates an nfs4_deviceid_put per reference and queues it on
+	 * @put_list for the caller to put and free.  On allocation failure
+	 * it must leave the reference in place.
+	 */
+	void (*reresolve_deviceid)(struct pnfs_layout_hdr *lo,
+				   const struct nfs4_deviceid *id,
+				   bool immediate,
+				   struct list_head *put_list);
+	/*
+	 * Does @lo hold any reference to deviceid @id?  Called under
+	 * @lo's inode i_lock; must not sleep.
+	 */
+	bool (*layout_references_deviceid)(struct pnfs_layout_hdr *lo,
+					   const struct nfs4_deviceid *id);
 
 	int (*prepare_layoutreturn) (struct nfs4_layoutreturn_args *);
 
 	void (*cleanup_layoutcommit) (struct nfs4_layoutcommit_data *data);
 	int (*prepare_layoutcommit) (struct nfs4_layoutcommit_args *args);
 	int (*prepare_layoutstats) (struct nfs42_layoutstat_args *args);
 
-	void (*cancel_io)(struct pnfs_layout_segment *lseg);
+	void (*cancel_io)(struct pnfs_layout_segment *lseg,
+			  const struct nfs4_deviceid *devid);
 };
 
 struct pnfs_commit_ops {
@@ -301,7 +321,8 @@ int pnfs_mark_matching_lsegs_invalid(struct pnfs_layout_hdr *lo,
 int pnfs_mark_matching_lsegs_return(struct pnfs_layout_hdr *lo,
 				struct list_head *tmp_list,
 				const struct pnfs_layout_range *recall_range,
-				u32 seq, bool cancel_io);
+				u32 seq, bool cancel_io,
+				const struct nfs4_deviceid *devid);
 int pnfs_mark_layout_stateid_invalid(struct pnfs_layout_hdr *lo,
 		struct list_head *lseg_list);
 bool pnfs_roc(struct inode *ino, struct nfs4_layoutreturn_args *args,
@@ -351,9 +372,56 @@ int pnfs_read_done_resend_to_mds(struct nfs_pgio_header *);
 int pnfs_write_done_resend_to_mds(struct nfs_pgio_header *);
 struct nfs4_threshold *pnfs_mdsthreshold_alloc(void);
 void pnfs_error_mark_layout_for_return(struct inode *inode,
-				       struct pnfs_layout_segment *lseg);
+				       struct pnfs_layout_segment *lseg,
+				       const struct nfs4_deviceid *devid);
 void pnfs_layout_return_unused_byclid(struct nfs_client *clp,
 				      enum pnfs_iomode iomode);
+void pnfs_layout_reresolve_deviceid_byclid(struct nfs_client *clp,
+				const struct pnfs_layoutdriver_type *ld,
+				const struct nfs4_deviceid *devid,
+				bool immediate);
+bool pnfs_layout_deviceid_referenced_byclid(struct nfs_client *clp,
+				const struct pnfs_layoutdriver_type *ld,
+				const struct nfs4_deviceid *devid);
+
+/*
+ * One live layout referencing a deviceID, collected for the
+ * CB_NOTIFY_DEVICEID DELETE recovery: the hdr is pinned, the inode
+ * igrab'd with its superblock active, and the layout stateid and
+ * cred snapshotted for TEST_STATEID.
+ */
+struct nfs4_deviceid_ref {
+	struct list_head node;
+	struct pnfs_layout_hdr *lo;
+	struct inode *inode;
+	nfs4_stateid stateid;
+	const struct cred *cred;
+};
+
+int pnfs_layout_collect_deviceid_refs(struct nfs_client *clp,
+				const struct pnfs_layoutdriver_type *ld,
+				const struct nfs4_deviceid *devid,
+				struct list_head *result);
+void pnfs_layout_put_deviceid_refs(struct list_head *result);
+
+/*
+ * A CB_NOTIFY_DEVICEID DELETE naming a deviceID that live layouts
+ * still reference (RFC 8881 Section 18.40.4).  Queued on
+ * nfs_client.cl_deviceid_deletes under cl_lock for the stat
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cancel_io`
- 外部函数：`int`
- 外部函数：`nfs4_decode_mp_ds_addr`
- 外部函数：`nfs4_delete_deviceid`
- 外部函数：`nfs4_deviceid_purge_client`
- 外部函数：`nfs4_find_get_deviceid`
- 外部函数：`nfs4_init_deviceid_node`
- 外部函数：`nfs4_mark_deviceid_available`
- 外部函数：`nfs4_mark_deviceid_unavailable`
- 外部函数：`nfs4_pnfs_ds_connect`
- 外部函数：`nfs4_put_deviceid_node`
- 外部函数：`nfs4_test_deviceid_unavailable`
- 外部函数：`pnfs_error_mark_layout_for_return`
- 外部函数：`pnfs_layout_mark_request_commit`
- 外部函数：`pnfs_layout_return_unused_byclid`
- 外部函数：`pnfs_mark_layout_stateid_invalid`
- 外部函数：`pnfs_mark_matching_lsegs_return`
- 外部函数：`pnfs_try_status`
- 外部函数：`pnfs_write_done_resend_to_mds`
- 外部函数：`void`
- 大写宏：`LAYOUTGET`
- 大写宏：`NFS`
- 大写宏：`NULL`
- 大写宏：`PNFS_ATTEMPTED`
- 大写宏：`PNFS_NOT_ATTEMPTED`
- 大写宏：`PNFS_TRY_AGAIN`
- 外部类型：`May`
- 外部类型：`RPCs`
- 外部类型：`Return`
- 外部类型：`atomic_t`
- 外部类型：`cred`
- 外部类型：`gfp_t`
- 外部类型：`hlist_node`
- 外部类型：`inode`
- 外部类型：`list_head`
- 外部类型：`loff_t`
- 外部类型：`module`
- 外部类型：`net`
- 外部类型：`nfs42_layoutstat_args`
- 外部类型：`nfs4_deviceid`
- 外部类型：`nfs4_layoutcommit_args`
- 外部类型：`nfs4_layoutcommit_data`
- 外部类型：`nfs4_layoutget_res`
- 外部类型：`nfs4_layoutreturn_args`
- 外部类型：`nfs4_layoutreturn_res`
- 外部类型：`nfs4_threshold`
- 外部类型：`nfs_client`
- 外部类型：`nfs_commit_info`
- 外部类型：`nfs_fh`
- 外部类型：`nfs_page`
- 外部类型：`nfs_pageio_ops`
- 外部类型：`nfs_pgio_header`
- 外部类型：`nfs_server`
- 外部类型：`page`
- 外部类型：`pnfs_ds_commit_info`
- 外部类型：`pnfs_iomode`
- 外部类型：`pnfs_layout_range`
- 外部类型：`rcu_head`
- 外部类型：`refcount_t`
- 外部类型：`size_t`
- 外部类型：`sockaddr_storage`
- 外部类型：`xdr_stream`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f89773aeb0` → 本草稿移入 `cases/defect/auto-linux-f89773aeb0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
