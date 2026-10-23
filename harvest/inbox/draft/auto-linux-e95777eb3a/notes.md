# auto-linux-e95777eb3a

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
| 外部依赖数（dep_count） | 10 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fcb92ecd733e11f37c32e7485da976a40d878e72 (https://github.com/torvalds/linux/commit/fcb92ecd733e11f37c32e7485da976a40d878e72)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 703；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fcb92ecd733e11f37c32e7485da976a40d878e72 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/linux/kernel/git/rdma/rdma :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -700,7 +700,8 @@ static int fill_res_cq_entry(struct sk_buff *msg, bool has_cap_net_admin,
 
 	if (nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CQN, res->id))
 		return -EMSGSIZE;
-	if (!rdma_is_kernel_res(res) &&
+
+	if (cq->uobject && cq->uobject->uevent.uobject.context &&
 	    nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CTXN,
 			cq->uobject->uevent.uobject.context->res.id))
 		return -EMSGSIZE;
@@ -790,7 +791,7 @@ static int fill_res_pd_entry(struct sk_buff *msg, bool has_cap_net_admin,
 	if (nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_PDN, res->id))
 		goto err;
 
-	if (!rdma_is_kernel_res(res) &&
+	if (pd->uobject && pd->uobject->context &&
 	    nla_put_u32(msg, RDMA_NLDEV_ATTR_RES_CTXN,
 			pd->uobject->context->res.id))
 		goto err;
@@ -1895,8 +1896,10 @@ static int nldev_dellink(struct sk_buff *skb, struct nlmsghdr *nlh,
 		mutex_lock(&nldev_dellink_mutex);
 		err = device->link_ops->dellink(device);
 		mutex_unlock(&nldev_dellink_mutex);
-		if (err)
+		if (err) {
+			ib_device_put(device);
 			return err;
+		}
 	}
 
 	ib_unregister_device_and_put(device);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dellink`
- 外部函数：`ib_unregister_device_and_put`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`nla_put_u32`
- 外部函数：`rdma_is_kernel_res`
- 大写宏：`EMSGSIZE`
- 大写宏：`RDMA_NLDEV_ATTR_RES_CQN`
- 大写宏：`RDMA_NLDEV_ATTR_RES_CTXN`
- 大写宏：`RDMA_NLDEV_ATTR_RES_PDN`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e95777eb3a` → 本草稿移入 `cases/defect/auto-linux-e95777eb3a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
