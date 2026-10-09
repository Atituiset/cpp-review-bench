# auto-linux-3195a90d25

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ff6f5157e034f81c2d6482259fc656cd0fe5a8ff](https://github.com/torvalds/linux/commit/ff6f5157e034f81c2d6482259fc656cd0fe5a8ff) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 165 |
| 编译错误数（gcc syntax-only） | 100（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ff6f5157e034f81c2d6482259fc656cd0fe5a8ff (https://github.com/torvalds/linux/commit/ff6f5157e034f81c2d6482259fc656cd0fe5a8ff)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 865；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ff6f5157e034f81c2d6482259fc656cd0fe5a8ff Merge branch 'mlxsw-fix-port-range-register-leak' :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -862,14 +862,19 @@ int mlxsw_sp_flower_tmplt_create(struct mlxsw_sp *mlxsw_sp,
 	memset(&rulei, 0, sizeof(rulei));
 	err = mlxsw_sp_flower_parse(mlxsw_sp, block, &rulei, f);
 	if (err)
-		return err;
+		goto out;
+
 	ruleset = mlxsw_sp_acl_ruleset_get(mlxsw_sp, block,
 					   f->common.chain_index,
 					   MLXSW_SP_ACL_PROFILE_FLOWER,
 					   &rulei.values.elusage);
+	err = PTR_ERR_OR_ZERO(ruleset);
+
+out:
+	mlxsw_sp_acl_rulei_free(mlxsw_sp, &rulei);
 
 	/* keep the reference to the ruleset */
-	return PTR_ERR_OR_ZERO(ruleset);
+	return err;
 }
 
 void mlxsw_sp_flower_tmplt_destroy(struct mlxsw_sp *mlxsw_sp,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`be16_to_cpu`
- 外部函数：`flow_action_first_entry_get`
- 外部函数：`flow_action_has_entries`
- 外部函数：`flow_action_is_last_entry`
- 外部函数：`flow_action_mixed_hw_stats_check`
- 外部函数：`mlxsw_sp_acl_dummy_fid`
- 外部函数：`mlxsw_sp_acl_rulei_act_count`
- 外部函数：`mlxsw_sp_acl_rulei_act_drop`
- 外部函数：`mlxsw_sp_acl_rulei_act_fid_set`
- 外部函数：`mlxsw_sp_acl_rulei_act_fwd`
- 外部函数：`mlxsw_sp_acl_rulei_act_ignore`
- 外部函数：`mlxsw_sp_acl_rulei_act_jump`
- 外部函数：`mlxsw_sp_acl_rulei_act_mirror`
- 外部函数：`mlxsw_sp_acl_rulei_act_priority`
- 外部函数：`mlxsw_sp_acl_rulei_act_terminate`
- 外部函数：`mlxsw_sp_acl_rulei_act_trap`
- 外部函数：`mlxsw_sp_acl_rulei_act_vlan`
- 外部函数：`mlxsw_sp_acl_ruleset_group_id`
- 外部函数：`mlxsw_sp_acl_ruleset_lookup`
- 外部函数：`mlxsw_sp_fid_index`
- 外部函数：`mlxsw_sp_flow_block_is_egress_bound`
- 外部函数：`mlxsw_sp_flow_block_is_ingress_bound`
- 外部函数：`mlxsw_sp_flow_block_is_mixed_bound`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`FLOW_ACTION_ACCEPT`
- 大写宏：`FLOW_ACTION_DROP`
- 大写宏：`FLOW_ACTION_GOTO`
- 大写宏：`FLOW_ACTION_HW_STATS_DISABLED`
- 大写宏：`FLOW_ACTION_HW_STATS_IMMEDIATE`
- 大写宏：`FLOW_ACTION_MANGLE`
- 大写宏：`FLOW_ACTION_MIRRED`
- 大写宏：`FLOW_ACTION_PIPE`
- 大写宏：`FLOW_ACTION_PRIORITY`
- 大写宏：`FLOW_ACTION_REDIRECT`
- 大写宏：`FLOW_ACTION_TRAP`
- 大写宏：`FLOW_ACTION_VLAN_MANGLE`
- 大写宏：`IS_ERR`
- 大写宏：`MLXSW_SP_ACL_PROFILE_FLOWER`
- 大写宏：`NL_SET_ERR_MSG_MOD`
- 大写宏：`PTR_ERR`
- 外部类型：`Cannot`
- 外部类型：`Count`
- 外部类型：`Drop`
- 外部类型：`Forbid`
- 外部类型：`Ignore`
- 外部类型：`Ingress`
- 外部类型：`Mirror`
- 外部类型：`Multiple`
- 外部类型：`Nothing`
- 外部类型：`Offload`
- 外部类型：`QoS`
- 外部类型：`Redirect`
- 外部类型：`Unsupported`
- 外部类型：`flow_action`
- 外部类型：`flow_action_entry`
- 外部类型：`flow_action_mangle_`
- 外部类型：`mlxsw_sp`
- 外部类型：`mlxsw_sp_acl_rule_info`
- 外部类型：`mlxsw_sp_acl_ruleset`
- 外部类型：`mlxsw_sp_fid`
- 外部类型：`mlxsw_sp_flow_block`
- 外部类型：`net_device`
- 外部类型：`netlink_ext_ack`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3195a90d25` → 本草稿移入 `cases/defect/auto-linux-3195a90d25/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
