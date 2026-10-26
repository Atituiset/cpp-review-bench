# auto-linux-ccde266a73

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
| 外部依赖数（dep_count） | 67 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 (https://github.com/torvalds/linux/commit/9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 651；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 Merge tag 'nfs-for-7.4-1' of git://git.linux-nfs.org/projects/anna/linux-nfs :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -15,6 +15,8 @@
 
 #include "nfs4session.h"
 #include "internal.h"
+#include <linux/hash.h>
+#include <linux/jhash.h>
 #include "pnfs.h"
 #include "netns.h"
 #include "nfs4trace.h"
@@ -55,6 +57,7 @@ void pnfs_generic_commit_release(void *calldata)
 	struct nfs_commit_data *data = calldata;
 
 	data->completion_ops->completion(data);
+	pnfs_put_ds_dev(data->ds_dev);
 	pnfs_put_lseg(data->lseg);
 	nfs_put_client(data->ds_clp);
 	nfs_commitdata_release(data);
@@ -576,8 +579,8 @@ same_sockaddr(struct sockaddr *addr1, struct sockaddr *addr2)
 }
 
 /*
- * Checks if 'dsaddrs1' contains a subset of 'dsaddrs2'. If it does,
- * declare a match.
+ * Checks if 'dsaddrs1' and 'dsaddrs2' hold the same set of addresses.
+ * If they do, declare a match.
  */
 static bool
 _same_data_server_addrs_locked(const struct list_head *dsaddrs1,
@@ -587,6 +590,10 @@ _same_data_server_addrs_locked(const struct list_head *dsaddrs1,
 	struct sockaddr *sa1, *sa2;
 	bool match = false;
 
+	if (list_count_nodes((struct list_head *)dsaddrs1) !=
+	    list_count_nodes((struct list_head *)dsaddrs2))
+		return false;
+
 	list_for_each_entry(da1, dsaddrs1, da_node) {
 		sa1 = (struct sockaddr *)&da1->da_addr;
 		match = false;
@@ -602,16 +609,58 @@ _same_data_server_addrs_locked(const struct list_head *dsaddrs1,
 	return match;
 }
 
+/* Hash family, address bytes, and port - as same_sockaddr() */
+static u32
+nfs4_ds_addr_hash(const struct sockaddr *sa)
+{
+	u32 h = sa->sa_family;
+
+	switch (sa->sa_family) {
+	case AF_INET: {
+		const struct sockaddr_in *a = (const struct sockaddr_in *)sa;
+
+		h = jhash(&a->sin_addr.s_addr, sizeof(a->sin_addr.s_addr), h);
+		h = jhash(&a->sin_port, sizeof(a->sin_port), h);
+		break;
+	}
+	case AF_INET6: {
+		const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)sa;
+
+		h = jhash(&a->sin6_addr, sizeof(a->sin6_addr), h);
+		h = jhash(&a->sin6_port, sizeof(a->sin6_port), h);
+		break;
+	}
+	}
+	return h;
+}
+
 /*
- * Lookup DS by addresses and NFS version.  nfs4_ds_cache_lock is held
+ * Bucket index for a DS cache key.  Per-address hashes combine by
+ * addition so the multipath list order cannot change the bucket,
+ * matching the order-independent set comparison above.
+ */
+static u32
+nfs4_ds_cache_hash(const struct list_head *dsaddrs, u32 version)
+{
+	const struct nfs4_pnfs_ds_addr *da;
+	u32 h = 0;
+
+	list_for_each_entry(da, dsaddrs, da_node)
+		h += nfs4_ds_addr_hash((const struct sockaddr *)&da->da_addr);
+	return hash_32(jhash_1word(version, h), NFS4_DS_CACHE_HASH_BITS);
+}
+
+/*
+ * Lookup DS by addresses and NFS version.  nfs4_data_server_lock is held
  */
 static struct nfs4_pnfs_ds *
 _data_server_lookup_locked(const struct nfs_net *nn,
 			   const struct list_head *dsaddrs, u32 version)
 {
 	struct nfs4_pnfs_ds *ds;
+	u32 bucket = nfs4_ds_cache_hash(dsaddrs, version);
 
-	list_for_each_entry(ds, &nn->nfs4_data_server_cache, ds_node)
+	hlist_for_each_entry(ds, &nn->nfs4_data_server_cache[bucket], ds_node)
 		if (ds->ds_version == version &&
 		    _same_data_server_addrs_locked(&ds->ds_addrs, dsaddrs))
 			return ds;
@@ -633,23 +682,28 @@ static void nfs4_pnfs_ds_addr_free(struct nfs4_pnfs_ds_addr *da)
 	kfree(da);
 }
 
-static void destroy_ds(struct nfs4_pnfs_ds *ds)
+void nfs4_pnfs_ds_addr_list_free(struct list_head *dsaddrs)
 {
 	struct nfs4_pnfs_ds_addr *da;
 
+	while (!list_empty(dsaddrs)) {
+		da = list_first_entry(dsaddrs, struct nfs4_pnfs_ds_addr,
+				      da_node);
+		list_del_init(&da->da_node);
+		nfs4_pnfs_ds_addr_free(da);
+	}
+}
+EXPORT_SYMBOL_GPL(nfs4_pnfs_ds_addr_list_free);
+
+static void destroy_ds(struct nfs4_pnfs_ds *ds)
+{
 	dprintk("--> %s\n", __func__);
 	ifdebug(FACILITY)
 		print_ds(ds);
 
 	nfs_put_client(ds->ds_clp);
 
-	while (!list_empty(&ds->ds_addrs)) {
-		da = list_first_entry(&ds->ds_addrs,
-				      struct nfs4_pnfs_ds_addr,
-				      da_node);
-		list_del_init(&da->da_node);
-		nfs4_pnfs_ds_addr_free(da);
-	}
+	nfs4_pnfs_ds_addr_list_free(&ds->ds_addrs);
 
 	kfree(ds
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_nfs4_pnfs_v4_ds_connect`
- 外部函数：`completion`
- 外部函数：`dprintk`
- 外部函数：`get_v3_ds_connect`
- 外部函数：`ifdebug`
- 外部函数：`ipv6_addr_equal`
- 外部函数：`ipv6_addr_src_scope`
- 外部函数：`kfree`
- 外部函数：`kzalloc`
- 外部函数：`list_add`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`list_splice_init`
- 外部函数：`memcpy`
- 外部函数：`net_generic`
- 外部函数：`nfs_commitdata_release`
- 外部函数：`nfs_put_client`
- 外部函数：`pnfs_put_lseg`
- 外部函数：`printk`
- 外部函数：`refcount_dec_and_lock`
- 外部函数：`refcount_read`
- 外部函数：`refcount_set`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`strlen`
- 大写宏：`AF_INET`
- 大写宏：`AF_INET6`
- 大写宏：`EIO`
- 大写宏：`ERR_PTR`
- 大写宏：`FACILITY`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`IPV6_ADDR_SCOPE_LINKLOCAL`
- 大写宏：`IS_ERR`
- 大写宏：`KERN_WARNING`
- 大写宏：`LINKLOCAL`
- 大写宏：`NFS`
- 大写宏：`NULL`
- 大写宏：`XPRT_TRANSPORT_TCP_TLS`
- 外部类型：`Checks`
- 外部类型：`If`
- 外部类型：`Lookup`
- 外部类型：`gfp_t`
- 外部类型：`list_head`
- 外部类型：`nfs4_pnfs_ds`
- 外部类型：`nfs4_pnfs_ds_addr`
- 外部类型：`nfs_client`
- 外部类型：`nfs_commit_data`
- 外部类型：`nfs_net`
- 外部类型：`nfs_server`
- 外部类型：`size_t`
- 外部类型：`sockaddr`
- 外部类型：`sockaddr_in`
- 外部类型：`sockaddr_in6`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ccde266a73` → 本草稿移入 `cases/defect/auto-linux-ccde266a73/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
