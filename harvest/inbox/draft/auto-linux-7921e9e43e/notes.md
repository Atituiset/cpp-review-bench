# auto-linux-7921e9e43e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4df4d2a577db2d3d57b888bcfb1206fc74661d16](https://github.com/torvalds/linux/commit/4df4d2a577db2d3d57b888bcfb1206fc74661d16) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4df4d2a577db2d3d57b888bcfb1206fc74661d16 (https://github.com/torvalds/linux/commit/4df4d2a577db2d3d57b888bcfb1206fc74661d16)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 107；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4df4d2a577db2d3d57b888bcfb1206fc74661d16 Merge tag 'acpi-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/rafael/linux-pm :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -28,6 +28,7 @@
  * to PCC commands
  */
 #define PCC_CMD_WAIT_RETRIES_NUM	500ULL
+#define PCC_SIGNATURE_SIZE		sizeof(u32)
 
 struct pcc_data {
 	struct pcc_mbox_chan *pcc_chan;
@@ -49,10 +50,24 @@ static acpi_status
 acpi_pcc_address_space_setup(acpi_handle region_handle, u32 function,
 			     void *handler_context,  void **region_context)
 {
-	struct pcc_data *data;
 	struct acpi_pcc_info *ctx = handler_context;
 	struct pcc_mbox_chan *pcc_chan;
+	struct pcc_data *data;
 	acpi_status ret;
+	u64 usecs_lat;
+
+	if (function == ACPI_REGION_DEACTIVATE) {
+		data = *region_context;
+		if (data) {
+			pcc_mbox_free_channel(data->pcc_chan);
+			kfree(data);
+			*region_context = NULL;
+		}
+		return AE_OK;
+	}
+
+	if (function != ACPI_REGION_ACTIVATE)
+		return AE_BAD_PARAMETER;
 
 	data = kzalloc_obj(*data);
 	if (!data)
@@ -74,13 +89,31 @@ acpi_pcc_address_space_setup(acpi_handle region_handle, u32 function,
 	}
 
 	pcc_chan = data->pcc_chan;
+	if (pcc_chan->shmem_size < PCC_SIGNATURE_SIZE ||
+	    ctx->length > pcc_chan->shmem_size - PCC_SIGNATURE_SIZE) {
+		pr_err("PCC channel-%d shared memory is too small.\n",
+		       ctx->subspace_id);
+		ret = AE_AML_REGION_LIMIT;
+		goto err_free_channel;
+	}
+
 	if (!pcc_chan->mchan->mbox->txdone_irq) {
 		pr_err("This channel-%d does not support interrupt.\n",
 		       ctx->subspace_id);
 		ret = AE_SUPPORT;
 		goto err_free_channel;
 	}
 
+	/*
+	 * pcc_chan->latency is just a Nominal value. In reality the remote
+	 * processor could be much slower to reply. So add an arbitrary
+	 * amount of wait on top of Nominal.
+	 */
+	usecs_lat = PCC_CMD_WAIT_RETRIES_NUM * pcc_chan->latency;
+	data->cl.tx_tout = DIV_ROUND_UP_ULL(usecs_lat, 1000);
+	if (!data->cl.tx_tout)
+		data->cl.tx_tout = 1;
+
 	*region_context = data;
 	return AE_OK;
 
@@ -97,35 +130,31 @@ acpi_pcc_address_space_handler(u32 function, acpi_physical_address addr,
 			       u32 bits, acpi_integer *value,
 			       void *handler_context, void *region_context)
 {
-	int ret;
 	struct pcc_data *data = region_context;
-	u64 usecs_lat;
+	void __iomem *pcc_opregion;
+	int ret;
+
+	pcc_opregion = data->pcc_chan->shmem + PCC_SIGNATURE_SIZE;
 
 	reinit_completion(&data->done);
 
-	/* Write to Shared Memory */
-	memcpy_toio(data->pcc_chan->shmem, (void *)value, data->ctx.length);
+	/* Write to the PCC OperationRegion after the shared memory signature. */
+	memcpy_toio(pcc_opregion, (void *)value, data->ctx.length);
 
 	ret = mbox_send_message(data->pcc_chan->mchan, NULL);
 	if (ret < 0)
 		return AE_ERROR;
 
-	/*
-	 * pcc_chan->latency is just a Nominal value. In reality the remote
-	 * processor could be much slower to reply. So add an arbitrary
-	 * amount of wait on top of Nominal.
-	 */
-	usecs_lat = PCC_CMD_WAIT_RETRIES_NUM * data->pcc_chan->latency;
 	ret = wait_for_completion_timeout(&data->done,
-						usecs_to_jiffies(usecs_lat));
+					  msecs_to_jiffies(data->cl.tx_tout));
 	if (ret == 0) {
 		pr_err("PCC command executed timeout!\n");
 		return AE_TIME;
 	}
 
 	mbox_chan_txdone(data->pcc_chan->mchan, ret);
 
-	memcpy_fromio(value, data->pcc_chan->shmem, data->ctx.length);
+	memcpy_fromio(value, pcc_opregion, data->ctx.length);
 
 	return AE_OK;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`kzalloc_obj`
- 外部函数：`mbox_chan_txdone`
- 外部函数：`mbox_send_message`
- 外部函数：`memcpy_fromio`
- 外部函数：`memcpy_toio`
- 外部函数：`pr_err`
- 外部函数：`reinit_completion`
- 外部函数：`usecs_to_jiffies`
- 外部函数：`wait_for_completion_timeout`
- 大写宏：`AE_ERROR`
- 大写宏：`AE_OK`
- 大写宏：`AE_SUPPORT`
- 大写宏：`AE_TIME`
- 大写宏：`NULL`
- 大写宏：`PCC`
- 外部类型：`In`
- 外部类型：`Memory`
- 外部类型：`Nominal`
- 外部类型：`Shared`
- 外部类型：`So`
- 外部类型：`This`
- 外部类型：`Write`
- 外部类型：`acpi_pcc_info`
- 外部类型：`pcc_mbox_chan`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7921e9e43e` → 本草稿移入 `cases/defect/auto-linux-7921e9e43e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
