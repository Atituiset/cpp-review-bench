# auto-linux-56827c4477

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#03741bcea0a9ec4c8f386fd5c6051c1a14070240](https://github.com/torvalds/linux/commit/03741bcea0a9ec4c8f386fd5c6051c1a14070240) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #03741bcea0a9ec4c8f386fd5c6051c1a14070240 (https://github.com/torvalds/linux/commit/03741bcea0a9ec4c8f386fd5c6051c1a14070240)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1331；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 03741bcea0a9ec4c8f386fd5c6051c1a14070240 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1328,7 +1328,7 @@ static int svm_vcpu_create(struct kvm_vcpu *vcpu)
 	svm->msrpm = svm_vcpu_alloc_msrpm();
 	if (!svm->msrpm) {
 		err = -ENOMEM;
-		goto error_free_sev;
+		goto error_free_avic;
 	}
 
 	svm->x2avic_msrs_intercepted = true;
@@ -1342,6 +1342,8 @@ static int svm_vcpu_create(struct kvm_vcpu *vcpu)
 
 	return 0;
 
+error_free_avic:
+	avic_vcpu_free(vcpu);
 error_free_sev:
 	sev_free_vcpu(vcpu);
 error_free_vmcb_page:
@@ -1356,6 +1358,8 @@ static void svm_vcpu_free(struct kvm_vcpu *vcpu)
 
 	WARN_ON_ONCE(!list_empty(&svm->ir_list));
 
+	avic_vcpu_free(vcpu);
+
 	svm_leave_nested(vcpu);
 	svm_free_nested(svm);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`list_empty`
- 外部函数：`sev_free_vcpu`
- 外部函数：`svm_free_nested`
- 外部函数：`svm_leave_nested`
- 外部函数：`svm_vcpu_alloc_msrpm`
- 大写宏：`ENOMEM`
- 大写宏：`WARN_ON_ONCE`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-56827c4477` → 本草稿移入 `cases/defect/auto-linux-56827c4477/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
