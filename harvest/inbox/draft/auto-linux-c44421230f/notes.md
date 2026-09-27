# auto-linux-c44421230f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#eff8d2791c086388ba5bae36385afd9bc6f0507e](https://github.com/torvalds/linux/commit/eff8d2791c086388ba5bae36385afd9bc6f0507e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 45 |
| 编译错误数（gcc syntax-only） | 39（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eff8d2791c086388ba5bae36385afd9bc6f0507e (https://github.com/torvalds/linux/commit/eff8d2791c086388ba5bae36385afd9bc6f0507e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 969；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eff8d2791c086388ba5bae36385afd9bc6f0507e Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -965,9 +965,14 @@ int kvm_riscv_aia_imsic_rw_attr(struct kvm *kvm, unsigned long type,
 	if (!vcpu)
 		return -ENODEV;
 
+	if (mutex_lock_killable(&vcpu->mutex))
+		return -EINTR;
+
 	imsic = vcpu->arch.aia_context.imsic_state;
-	if (!imsic)
-		return -ENODEV;
+	if (!imsic) {
+		rc = -ENODEV;
+		goto out_unlock;
+	}
 	isel = KVM_DEV_RISCV_AIA_IMSIC_GET_ISEL(type);
 
 	read_lock_irqsave(&imsic->vsfile_lock, flags);
@@ -991,6 +996,8 @@ int kvm_riscv_aia_imsic_rw_attr(struct kvm *kvm, unsigned long type,
 		rc = imsic_vsfile_rw(vsfile_hgei, vsfile_cpu, imsic->nr_eix,
 				     isel, write, val);
 
+out_unlock:
+	mutex_unlock(&vcpu->mutex);
 	return rc;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpumask_of`
- 外部函数：`csr_read`
- 外部函数：`csr_write`
- 外部函数：`file`
- 外部函数：`imsic_eix_read`
- 外部函数：`imsic_eix_write`
- 外部函数：`imsic_read_switchcase_16`
- 外部函数：`imsic_read_switchcase_64`
- 外部函数：`imsic_vs_csr_read`
- 外部函数：`imsic_vs_csr_write`
- 外部函数：`imsic_write_switchcase_16`
- 外部函数：`imsic_write_switchcase_32`
- 外部函数：`imsic_write_switchcase_64`
- 外部函数：`on_each_cpu_mask`
- 外部函数：`read_lock_irqsave`
- 大写宏：`BITS_PER_LONG`
- 大写宏：`BITS_PER_TYPE`
- 大写宏：`CONFIG_32BIT`
- 大写宏：`CPU`
- 大写宏：`CSR_HSTATUS`
- 大写宏：`CSR_VSIREG`
- 大写宏：`CSR_VSISELECT`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENOENT`
- 大写宏：`HSTATUS_VGEIN`
- 大写宏：`HSTATUS_VGEIN_SHIFT`
- 大写宏：`IMSIC`
- 大写宏：`IMSIC_EIDELIVERY`
- 大写宏：`IMSIC_EIE0`
- 大写宏：`IMSIC_EIE63`
- 大写宏：`IMSIC_EIP0`
- 大写宏：`IMSIC_EIP63`
- 大写宏：`IMSIC_EITHRESHOLD`
- 大写宏：`IMSIC_MAX_ID`
- 大写宏：`KVM_DEV_RISCV_AIA_IMSIC_GET_ISEL`
- 外部类型：`At`
- 外部类型：`Check`
- 外部类型：`Hardware`
- 外部类型：`Software`
- 外部类型：`We`
- 外部类型：`kvm_io_device`
- 外部类型：`phys_addr_t`
- 外部类型：`raw_spinlock_t`
- 外部类型：`rwlock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c44421230f` → 本草稿移入 `cases/defect/auto-linux-c44421230f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
