# auto-linux-c6a6ef9990

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#89d2d7bf0698b7fdfc914bfd219e739e9308d332](https://github.com/torvalds/linux/commit/89d2d7bf0698b7fdfc914bfd219e739e9308d332) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #89d2d7bf0698b7fdfc914bfd219e739e9308d332 (https://github.com/torvalds/linux/commit/89d2d7bf0698b7fdfc914bfd219e739e9308d332)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 894；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 89d2d7bf0698b7fdfc914bfd219e739e9308d332 Merge tag 'efi-next-for-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/efi/efi :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -891,20 +891,21 @@ bool efi_is_table_address(unsigned long phys_addr)
 
 #define EFI_ATTR_SHOW(name) \
 static ssize_t name##_show(struct kobject *kobj, \
-				struct kobj_attribute *attr, char *buf) \
+			   const struct kobj_attribute *attr, char *buf) \
 { \
-	return sprintf(buf, "0x%lx\n", EFI_FIELD(name)); \
+	return sysfs_emit(buf, "0x%lx\n", EFI_FIELD(name)); \
 }
 
 EFI_ATTR_SHOW(fw_vendor);
 EFI_ATTR_SHOW(runtime);
 EFI_ATTR_SHOW(config_table);
 
-struct kobj_attribute efi_attr_fw_vendor = __ATTR_RO(fw_vendor);
-struct kobj_attribute efi_attr_runtime = __ATTR_RO(runtime);
-struct kobj_attribute efi_attr_config_table = __ATTR_RO(config_table);
+const struct kobj_attribute efi_attr_fw_vendor = __KOBJ_ATTR_RO(fw_vendor);
+const struct kobj_attribute efi_attr_runtime = __KOBJ_ATTR_RO(runtime);
+const struct kobj_attribute efi_attr_config_table = __KOBJ_ATTR_RO(config_table);
 
-umode_t efi_attr_is_visible(struct kobject *kobj, struct attribute *attr, int n)
+umode_t efi_attr_is_visible(struct kobject *kobj, const struct attribute *attr,
+			    int n)
 {
 	if (attr == &efi_attr_fw_vendor.attr) {
 		if (efi_enabled(EFI_PARAVIRT) ||
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ATTR_RO`
- 外部函数：`_show`
- 外部函数：`efi_enabled`
- 外部函数：`sprintf`
- 大写宏：`EFI_PARAVIRT`
- 外部类型：`attribute`
- 外部类型：`kobj_attribute`
- 外部类型：`kobject`
- 外部类型：`ssize_t`
- 外部类型：`umode_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-c6a6ef9990` → 本草稿移入 `cases/defect/auto-linux-c6a6ef9990/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
