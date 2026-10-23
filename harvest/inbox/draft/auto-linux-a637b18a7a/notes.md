# auto-linux-a637b18a7a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#1021679bed5acc46e1b2dac78bdd006bd50970f4](https://github.com/torvalds/linux/commit/1021679bed5acc46e1b2dac78bdd006bd50970f4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 43 |
| 编译错误数（gcc syntax-only） | 35（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1021679bed5acc46e1b2dac78bdd006bd50970f4 (https://github.com/torvalds/linux/commit/1021679bed5acc46e1b2dac78bdd006bd50970f4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 382；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1021679bed5acc46e1b2dac78bdd006bd50970f4 Merge tag 'for-linus-iommufd' of git://git.kernel.org/pub/scm/linux/kernel/git/jgg/iommufd :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -322,6 +322,106 @@ struct arm_vsmmu_invalidation_cmd {
 	};
 };
 
+/* Reject the range field values that the spec defines as Reserved */
+static int arm_vsmmu_validate_range(struct arm_smmu_device *smmu, u64 data[2])
+{
+	bool range = !!(data[0] & (CMDQ_TLBI_0_NUM | CMDQ_TLBI_0_SCALE));
+	u8 ttl = FIELD_GET(CMDQ_TLBI_1_TTL, data[1]);
+	u8 tg = FIELD_GET(CMDQ_TLBI_1_TG, data[1]);
+
+	/* NUM, SCALE and TTL are RES0 when TG == 0 */
+	if (!tg)
+		return (range || ttl) ? -EIO : 0;
+	/* TTL == 0b01 with a 16KB TG requires SMMU_IDR5.DS */
+	if (tg == 2 && ttl == 1 && !(smmu->features & ARM_SMMU_FEAT_DS))
+		return -EIO;
+	/* NUM == 0, SCALE == 0 with TTL == 0 is a reserved combination */
+	if (!range && !ttl)
+		return -EIO;
+	return 0;
+}
+
+static int arm_vsmmu_validate_user_cmd(struct arm_vsmmu *vsmmu, u64 data[2])
+{
+	struct arm_smmu_device *smmu = vsmmu->smmu;
+	u64 allowed[2] = { CMDQ_0_OP };
+
+	/* Collect the fields userspace is allowed to set for each opcode */
+	switch (data[0] & CMDQ_0_OP) {
+	case CMDQ_OP_TLBI_NSNH_ALL:
+		break;
+	case CMDQ_OP_TLBI_NH_VA:
+		/* An SMMU with 8-bit ASIDs treats the upper 8 bits as RES0 */
+		allowed[0] |= FIELD_PREP(CMDQ_TLBI_0_ASID,
+					 GENMASK(smmu->asid_bits - 1, 0));
+		fallthrough;
+	case CMDQ_OP_TLBI_NH_VAA:
+		/* NUM/SCALE/TG/TTL are range fields gated on FEAT_RANGE_INV */
+		if (smmu->features & ARM_SMMU_FEAT_RANGE_INV) {
+			if (arm_vsmmu_validate_range(smmu, data))
+				return -EIO;
+			allowed[0] |= CMDQ_TLBI_0_NUM | CMDQ_TLBI_0_SCALE;
+			allowed[1] |= CMDQ_TLBI_1_TG | CMDQ_TLBI_1_TTL;
+			/* SCALE bit 25 (values above 31) is RES0 without DS */
+			if (!(smmu->features & ARM_SMMU_FEAT_DS))
+				allowed[0] &= ~FIELD_PREP(CMDQ_TLBI_0_SCALE,
+							  BIT(5));
+		}
+		allowed[0] |= CMDQ_TLBI_0_VMID;
+		allowed[1] |= CMDQ_TLBI_1_LEAF | CMDQ_TLBI_1_VA_MASK;
+		break;
+	case CMDQ_OP_TLBI_NH_ASID:
+		/* An SMMU with 8-bit ASIDs treats the upper 8 bits as RES0 */
+		allowed[0] |= FIELD_PREP(CMDQ_TLBI_0_ASID,
+					 GENMASK(smmu->asid_bits - 1, 0));
+		fallthrough;
+	case CMDQ_OP_TLBI_NH_ALL:
+		allowed[0] |= CMDQ_TLBI_0_VMID;
+		break;
+	case CMDQ_OP_ATC_INV:
+		/* ATC_INV is illegal unless the SMMU implements ATS */
+		if (!(smmu->features & ARM_SMMU_FEAT_ATS))
+			return -EIO;
+		/* A Size above 52 (invalidate-all) may raise a CERROR_ILL */
+		if (FIELD_GET(CMDQ_ATC_1_SIZE, data[1]) > ATC_INV_SIZE_ALL)
+			return -EIO;
+		/*
+		 * SSV/SSID/Global need substream support. SSID and Global are
+		 * IGNORED (not RES0) when SSV == 0, so they need no SSV check.
+		 */
+		if (smmu->ssid_bits)
+			allowed[0] |= CMDQ_0_SSV | CMDQ_ATC_0_SSID |
+				      CMDQ_ATC_0_GLOBAL;
+		allowed[0] |= CMDQ_ATC_0_SID;
+		allowed[1] |= CMDQ_ATC_1_SIZE | CMDQ_ATC_1_ADDR_MASK;
+		break;
+	case CMDQ_OP_CFGI_CD:
+		/* No SSV for CFGI_CD; SSID requires substream support */
+		if (smmu->ssid_bits)
+			allowed[0] |= CMDQ_CFGI_0_SSID;
+		allowed[1] |= CMDQ_CFGI_1_LEAF;
+		fallthrough;
+	case CMDQ_OP_CFGI_CD_ALL:
+		allowed[0] |= CMDQ_CFGI_0_SID;
+		break;
+	default:
+		return -EIO;
+	}
+
+	/*
+	 * Reject any other bit, e.g. a RES0 bit or a Secure bit, before the
+	 * command reaches the trusted main cmdq, so a guest cannot wedge the
+	 * shared queue for every device with a CERROR_ILL.
+	 *
+	 * By contrast, an out-of-range address or ID value does not need a
+	 * check: the spec defines it as CONSTRAINED UNPREDICTABLE, which is
+	 * scoped to the guest itself and does not raise a CERROR_ILL.
+	 */
+	if ((data[0] & ~allowed[0]) || (data[1] & ~allowed[1]))
+		return -EIO;
+	return 0;
+}
+
 /*
  * Convert, in place, the raw invalidation command into an internal format that
  * can be passed to arm_smmu_cmdq_issue_cmdlist(). Internally commands are
@@ -332,33 +432,40 @@ struct arm_vsmmu_invalidation_cmd {
 static int arm_vsmmu_convert_user_cmd(struct arm_vsmmu *vsmmu,
 				      struct arm_vsmmu_invalidation_cmd *cmd)
 {
+	u64 *data = cmd->cmd.data;
+	int ret;
+
 	/* Commands are le64 stored in u64 */
-	cmd->cm
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__arm_smmu_cmdq_issue_cmdlist`
- 外部函数：`arm_vsmmu_convert_user_cmd`
- 外部函数：`container_of`
- 外部函数：`dev_iommu_priv_get`
- 外部函数：`iommu_copy_struct_from_full_user_array`
- 外部函数：`iommufd_viommu_find_dev`
- 外部函数：`kfree`
- 外部函数：`kzalloc_objs`
- 外部函数：`le64_to_cpu`
- 外部函数：`xa_lock`
- 外部函数：`xa_unlock`
- 大写宏：`CMDQ_0_OP`
- 大写宏：`CMDQ_BATCH_ENTRIES`
- 大写宏：`CMDQ_CFGI_0_SID`
- 大写宏：`CMDQ_OP_ATC_INV`
- 大写宏：`CMDQ_OP_CFGI_CD`
- 大写宏：`CMDQ_OP_CFGI_CD_ALL`
- 大写宏：`CMDQ_OP_TLBI_NH_ALL`
- 大写宏：`CMDQ_OP_TLBI_NH_ASID`
- 大写宏：`CMDQ_OP_TLBI_NH_VA`
- 大写宏：`CMDQ_OP_TLBI_NH_VAA`
- 大写宏：`CMDQ_OP_TLBI_NSNH_ALL`
- 大写宏：`CMDQ_TLBI_0_VMID`
- 大写宏：`EIO`
- 大写宏：`ENOMEM`
- 大写宏：`FIELD_GET`
- 大写宏：`FIELD_PREP`
- 大写宏：`FIXME`
- 大写宏：`IOMMU_VIOMMU_INVALIDATE_DATA_ARM_SMMUV3`
- 大写宏：`NH_ALL`
- 大写宏：`PCI`
- 大写宏：`SID`
- 外部类型：`At`
- 外部类型：`Commands`
- 外部类型：`Convert`
- 外部类型：`Internally`
- 外部类型：`arm_smmu_cmd`
- 外部类型：`arm_smmu_device`
- 外部类型：`arm_smmu_master`
- 外部类型：`arm_vsmmu`
- 外部类型：`device`
- 外部类型：`iommu_user_data_array`
- 外部类型：`iommu_viommu_arm_smmuv3_invalidate`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a637b18a7a` → 本草稿移入 `cases/defect/auto-linux-a637b18a7a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
