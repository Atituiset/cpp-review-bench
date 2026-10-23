# auto-linux-dc22fb2db3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#fcb92ecd733e11f37c32e7485da976a40d878e72](https://github.com/torvalds/linux/commit/fcb92ecd733e11f37c32e7485da976a40d878e72) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 33 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fcb92ecd733e11f37c32e7485da976a40d878e72 (https://github.com/torvalds/linux/commit/fcb92ecd733e11f37c32e7485da976a40d878e72)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4841；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fcb92ecd733e11f37c32e7485da976a40d878e72 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/linux/kernel/git/rdma/rdma :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4823,6 +4823,29 @@ int rdma_reject(struct rdma_cm_id *id, const void *private_data,
 }
 EXPORT_SYMBOL(rdma_reject);
 
+static int cma_force_disconnect(struct rdma_id_private *id_priv)
+{
+	struct cma_work *work;
+
+	work = kzalloc_obj(*work);
+	if (!work)
+		return -ENOMEM;
+
+	INIT_WORK(&work->work, cma_work_handler);
+	work->old_state = RDMA_CM_CONNECT;
+	work->new_state = RDMA_CM_DISCONNECT;
+	work->event.event = RDMA_CM_EVENT_DISCONNECTED;
+	work->event.status = -ECONNABORTED;
+
+	cma_id_get(id_priv);
+	work->id = id_priv;
+
+	trace_cm_force_disconnect(id_priv);
+
+	queue_work(cma_wq, &work->work);
+	return 0;
+}
+
 int rdma_disconnect(struct rdma_cm_id *id)
 {
 	struct rdma_id_private *id_priv;
@@ -4838,12 +4861,15 @@ int rdma_disconnect(struct rdma_cm_id *id)
 			goto out;
 		/* Initiate or respond to a disconnect. */
 		trace_cm_disconnect(id_priv);
-		if (ib_send_cm_dreq(id_priv->cm_id.ib, NULL, 0)) {
-			if (!ib_send_cm_drep(id_priv->cm_id.ib, NULL, 0))
-				trace_cm_sent_drep(id_priv);
-		} else {
+		if (!ib_send_cm_dreq(id_priv->cm_id.ib, NULL, 0)) {
 			trace_cm_sent_dreq(id_priv);
+			goto out;
+		}
+		if (!ib_send_cm_drep(id_priv->cm_id.ib, NULL, 0)) {
+			trace_cm_sent_drep(id_priv);
+			goto out;
 		}
+		ret = cma_force_disconnect(id_priv);
 	} else if (rdma_cap_iw_cm(id->device, id->port_num)) {
 		ret = iw_cm_disconnect(id_priv->cm_id.iw, 0);
 	} else
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`be32_to_cpu`
- 外部函数：`container_of`
- 外部函数：`ib_addr_get_mgid`
- 外部函数：`ib_sa_get_mcmember_rec`
- 外部函数：`ib_send_cm_drep`
- 外部函数：`ib_send_cm_dreq`
- 外部函数：`ib_send_cm_rej`
- 外部函数：`ib_send_cm_sidr_rep`
- 外部函数：`iw_cm_disconnect`
- 外部函数：`iw_cm_reject`
- 外部函数：`memset`
- 外部函数：`rdma_cap_ib_cm`
- 外部函数：`rdma_cap_iw_cm`
- 外部函数：`trace_cm_disconnect`
- 外部函数：`trace_cm_send_rej`
- 外部函数：`trace_cm_send_sidr_rep`
- 外部函数：`trace_cm_sent_drep`
- 外部函数：`trace_cm_sent_dreq`
- 大写宏：`EINVAL`
- 大写宏：`ENOSYS`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`IB_QPT_UD`
- 大写宏：`IB_SIDR_REJECT`
- 大写宏：`IB_SIDR_SUCCESS`
- 大写宏：`NULL`
- 大写宏：`RDMA_PS_IB`
- 大写宏：`RDMA_PS_IPOIB`
- 大写宏：`RDMA_PS_UDP`
- 大写宏：`RDMA_UDP_QKEY`
- 外部类型：`Initiate`
- 外部类型：`ib_cm_sidr_rep_param`
- 外部类型：`ib_cm_sidr_status`
- 外部类型：`ib_sa_mcmember_rec`
- 外部类型：`rdma_cm_id`
- 外部类型：`rdma_id_private`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-dc22fb2db3` → 本草稿移入 `cases/defect/auto-linux-dc22fb2db3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
