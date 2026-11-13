# auto-linux-0382d6eed2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#dd6f1e7582f0f7f12c2d620cf50446aa987d7327](https://github.com/torvalds/linux/commit/dd6f1e7582f0f7f12c2d620cf50446aa987d7327) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dd6f1e7582f0f7f12c2d620cf50446aa987d7327 (https://github.com/torvalds/linux/commit/dd6f1e7582f0f7f12c2d620cf50446aa987d7327)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 273；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dd6f1e7582f0f7f12c2d620cf50446aa987d7327 net: netns: fix INT_MAX nsid cleanup :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -270,14 +270,19 @@ static void ops_undo_single(struct pernet_operations *ops,
 /* should be called with nsid_lock held */
 static int alloc_netid(struct net *net, struct net *peer, int reqid)
 {
-	int min = 0, max = 0;
+	int ret;
+	u32 id;
 
 	if (reqid >= 0) {
-		min = reqid;
-		max = reqid + 1;
+		id = reqid;
+		ret = idr_alloc_u32(&net->netns_ids, peer, &id, id,
+				    GFP_ATOMIC);
+		if (ret)
+			return ret;
+		return id;
 	}
 
-	return idr_alloc(&net->netns_ids, peer, min, max, GFP_ATOMIC);
+	return idr_alloc(&net->netns_ids, peer, 0, 0, GFP_ATOMIC);
 }
 
 /* This function is used by idr_for_each(). If net is equal to peer, the
@@ -639,10 +644,10 @@ static void unhash_nsid(struct net *last)
 	 * use for_each_net_rcu() or net_rwsem.
 	 */
 	for_each_net(tmp) {
-		int id = 0;
+		unsigned long id = 0;
 
 		spin_lock(&tmp->nsid_lock);
-		while ((peer = idr_get_next(&tmp->netns_ids, &id))) {
+		while ((peer = idr_get_next_ul(&tmp->netns_ids, &id))) {
 			int curr_id = id;
 
 			id++;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`for_each_net`
- 外部函数：`for_each_net_rcu`
- 外部函数：`idr_alloc`
- 外部函数：`idr_get_next`
- 外部函数：`spin_lock`
- 大写宏：`GFP_ATOMIC`
- 外部类型：`If`
- 外部类型：`This`
- 外部类型：`net`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0382d6eed2` → 本草稿移入 `cases/defect/auto-linux-0382d6eed2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
