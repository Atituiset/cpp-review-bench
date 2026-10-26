# auto-linux-36baff6d12

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
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 (https://github.com/torvalds/linux/commit/9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 185；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9e089e670e9ad1caf4a5b2b63e01b2dc076efbe3 Merge tag 'nfs-for-7.4-1' of git://git.linux-nfs.org/projects/anna/linux-nfs :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -177,27 +177,14 @@ nfs4_fl_alloc_deviceid_node(struct nfs_server *server, struct pnfs_device *pdev,
 		trace_fl_getdevinfo(server, &pdev->dev_id, dsaddr->ds_list[i]->ds_remotestr);
 
 		/* If DS was already in cache, free ds addrs */
-		while (!list_empty(&dsaddrs)) {
-			da = list_first_entry(&dsaddrs,
-					      struct nfs4_pnfs_ds_addr,
-					      da_node);
-			list_del_init(&da->da_node);
-			kfree(da->da_remotestr);
-			kfree(da);
-		}
+		nfs4_pnfs_ds_addr_list_free(&dsaddrs);
 	}
 
 	folio_put(scratch);
 	return dsaddr;
 
 out_err_drain_dsaddrs:
-	while (!list_empty(&dsaddrs)) {
-		da = list_first_entry(&dsaddrs, struct nfs4_pnfs_ds_addr,
-				      da_node);
-		list_del_init(&da->da_node);
-		kfree(da->da_remotestr);
-		kfree(da);
-	}
+	nfs4_pnfs_ds_addr_list_free(&dsaddrs);
 out_err_free_deviceid:
 	nfs4_fl_free_deviceid(dsaddr);
 	/* stripe_indicies was part of dsaddr */
@@ -280,7 +267,7 @@ nfs4_fl_prepare_ds(struct pnfs_layout_segment *lseg, u32 ds_idx)
 		goto out_test_devid;
 
 	status = nfs4_pnfs_ds_connect(s, ds, devid, dataserver_timeo,
-			     dataserver_retrans, 4,
+			     dataserver_retrans, 0, 4,
 			     s->nfs_client->cl_minorversion, true);
 	if (status) {
 		nfs4_mark_deviceid_unavailable(devid);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`folio_put`
- 外部函数：`kfree`
- 外部函数：`kfree_rcu`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`nfs4_mark_deviceid_unavailable`
- 外部函数：`nfs4_pnfs_ds_connect`
- 外部函数：`nfs4_pnfs_ds_put`
- 外部函数：`nfs4_print_deviceid`
- 外部函数：`trace_fl_getdevinfo`
- 大写宏：`NULL`
- 外部类型：`If`
- 外部类型：`nfs4_file_layout_dsaddr`
- 外部类型：`nfs4_pnfs_ds`
- 外部类型：`nfs4_pnfs_ds_addr`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-36baff6d12` → 本草稿移入 `cases/defect/auto-linux-36baff6d12/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
