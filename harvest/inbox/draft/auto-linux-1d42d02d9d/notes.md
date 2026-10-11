# auto-linux-1d42d02d9d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a5ebb76233b79db01e82e061173cb73c6d2b5c6b](https://github.com/torvalds/linux/commit/a5ebb76233b79db01e82e061173cb73c6d2b5c6b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 29（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a5ebb76233b79db01e82e061173cb73c6d2b5c6b (https://github.com/torvalds/linux/commit/a5ebb76233b79db01e82e061173cb73c6d2b5c6b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2805；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a5ebb76233b79db01e82e061173cb73c6d2b5c6b Merge tag 'edac_urgent_for_v7.3_rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/ras/ras :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2796,13 +2796,13 @@ static inline void decode_bus_error(int node_id, struct mce *m)
  * the instance_id. For example, instance_id=0xYXXXXX where Y is the channel
  * number.
  *
- * For DRAM ECC errors, the Chip Select number is given in bits [2:0] of
+ * For DRAM ECC errors, the Chip Select number is given in bits [1:0] of
  * the MCA_SYND[ErrorInformation] field.
  */
 static void umc_get_err_info(struct mce *m, struct err_info *err)
 {
 	err->channel = (m->ipid & GENMASK(31, 0)) >> 20;
-	err->csrow = m->synd & 0x7;
+	err->csrow = m->synd & 0x3;
 }
 
 static void decode_umc_error(int node_id, struct mce *m)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`amd_convert_umc_mca_addr_to_sys_addr`
- 外部函数：`edac_mc_find`
- 外部函数：`edac_mc_handle_error`
- 外部函数：`get_err_info`
- 外部函数：`memset`
- 外部函数：`smca_get_bank_type`
- 大写宏：`AMD`
- 大写宏：`CPU`
- 大写宏：`DECODE_OK`
- 大写宏：`DRAM`
- 大写宏：`ECC`
- 大写宏：`ERR_CHANNEL`
- 大写宏：`ERR_CSROW`
- 大写宏：`ERR_NODE`
- 大写宏：`ERR_NORM_ADDR`
- 大写宏：`ERR_SYND`
- 大写宏：`GENMASK`
- 大写宏：`GPU`
- 大写宏：`HW_EVENT_ERR_CORRECTED`
- 大写宏：`HW_EVENT_ERR_DEFERRED`
- 大写宏：`HW_EVENT_ERR_UNCORRECTED`
- 大写宏：`IS_ERR_VALUE`
- 大写宏：`MCA_IPID`
- 大写宏：`MCA_SYND`
- 大写宏：`MCI_STATUS_DEFERRED`
- 大写宏：`MCI_STATUS_SYNDV`
- 大写宏：`PAGE_MASK`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`SMCA_UMC_V2`
- 大写宏：`WARN`
- 大写宏：`WTF`
- 外部类型：`Cannot`
- 外部类型：`Chip`
- 外部类型：`Convert`
- 外部类型：`Denmark`
- 外部类型：`ErrorInformation`
- 外部类型：`Failed`
- 外部类型：`For`
- 外部类型：`InstanceIdHi`
- 外部类型：`Linux`
- 外部类型：`Node`
- 外部类型：`Nodes`
- 外部类型：`Select`
- 外部类型：`Something`
- 外部类型：`Unknown`
- 外部类型：`amd64_pvt`
- 外部类型：`atl_err`
- 外部类型：`err_info`
- 外部类型：`hw_event_mc_err_type`
- 外部类型：`mce`
- 外部类型：`mem_ctl_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1d42d02d9d` → 本草稿移入 `cases/defect/auto-linux-1d42d02d9d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
