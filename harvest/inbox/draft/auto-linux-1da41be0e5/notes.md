# auto-linux-1da41be0e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a940b03cee1524c10c16e0f73ec878bbd36202a2](https://github.com/torvalds/linux/commit/a940b03cee1524c10c16e0f73ec878bbd36202a2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a940b03cee1524c10c16e0f73ec878bbd36202a2 (https://github.com/torvalds/linux/commit/a940b03cee1524c10c16e0f73ec878bbd36202a2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 44（原始 PR diff 行 2389；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a940b03cee1524c10c16e0f73ec878bbd36202a2 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2386,9 +2386,6 @@ int __init populate_nv_trap_config(void)
 				print_nv_trap_error(fgt, "FGT bit is reserved", ret);
 			}
 
-			if (!cpus_have_final_cap(ARM64_HAS_FGT))
-				continue;
-
 			prev = xa_store(&sr_forward_xa, enc,
 					xa_mk_value(tc.val), GFP_KERNEL);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpus_have_final_cap`
- 外部函数：`kvm_err`
- 外部函数：`sys_reg_CRm`
- 外部函数：`sys_reg_CRn`
- 外部函数：`sys_reg_Op0`
- 外部函数：`sys_reg_Op1`
- 外部函数：`sys_reg_Op2`
- 外部函数：`xa_mk_value`
- 外部函数：`xa_store`
- 大写宏：`ARM64_HAS_FGT`
- 大写宏：`FGT`
- 大写宏：`GFP_KERNEL`
- 外部类型：`Be`
- 外部类型：`Bit`
- 外部类型：`Coarse`
- 外部类型：`Filter`
- 外部类型：`Fine`
- 外部类型：`Grained`
- 外部类型：`Index`
- 外部类型：`Must`
- 外部类型：`Polarity`
- 外部类型：`SysReg`
- 外部类型：`Trap`
- 外部类型：`Unused`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1da41be0e5` → 本草稿移入 `cases/defect/auto-linux-1da41be0e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
