# auto-linux-dd7b4a120c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#331eadd85f14bac85663c14c99228f6dedf64d87](https://github.com/torvalds/linux/commit/331eadd85f14bac85663c14c99228f6dedf64d87) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #331eadd85f14bac85663c14c99228f6dedf64d87 (https://github.com/torvalds/linux/commit/331eadd85f14bac85663c14c99228f6dedf64d87)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 193；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 331eadd85f14bac85663c14c99228f6dedf64d87 Merge tag 'selinux-pr-20261015' of git://git.kernel.org/pub/scm/linux/kernel/git/pcmoore/selinux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -190,8 +190,8 @@ avc_xperms_decision_lookup(u8 driver, u8 base_perm,
 }
 
 static inline unsigned int
-avc_xperms_has_perm(struct extended_perms_decision *xpd,
-					u8 perm, u8 which)
+avc_xperms_has_perm(const struct extended_perms_decision *xpd,
+		    u8 perm, u8 which)
 {
 	unsigned int rc = 0;
 
@@ -247,7 +247,7 @@ static void avc_xperms_free(struct avc_xperms_node *xp_node)
 }
 
 static void avc_copy_xperms_decision(struct extended_perms_decision *dest,
-					struct extended_perms_decision *src)
+				     const struct extended_perms_decision *src)
 {
 	dest->base_perm = src->base_perm;
 	dest->driver = src->driver;
@@ -269,7 +269,7 @@ static void avc_copy_xperms_decision(struct extended_perms_decision *dest,
  */
 static inline void avc_quick_copy_xperms_decision(u8 perm,
 			struct extended_perms_decision *dest,
-			struct extended_perms_decision *src)
+			const struct extended_perms_decision *src)
 {
 	/*
 	 * compute index of the u32 of the 256 bits (8 u32s) that contain this
@@ -323,7 +323,7 @@ static struct avc_xperms_decision_node
 }
 
 static int avc_add_xperms_decision(struct avc_node *node,
-			struct extended_perms_decision *src)
+				   const struct extended_perms_decision *src)
 {
 	struct avc_xperms_decision_node *dest_xpd;
 
@@ -348,7 +348,7 @@ static struct avc_xperms_node *avc_xperms_alloc(void)
 }
 
 static int avc_xperms_populate(struct avc_node *node,
-				struct avc_xperms_node *src)
+			       const struct avc_xperms_node *src)
 {
 	struct avc_xperms_node *dest;
 	struct avc_xperms_decision_node *dest_xpd;
@@ -381,14 +381,17 @@ static int avc_xperms_populate(struct avc_node *node,
 }
 
 static inline u32 avc_xperms_audit_required(u32 requested,
-					struct av_decision *avd,
-					struct extended_perms_decision *xpd,
-					u8 perm,
-					int result,
-					u32 *deniedp)
+				const struct av_decision *avd,
+				const struct extended_perms_decision *xpd,
+				u8 perm,
+				int result,
+				u32 *deniedp)
 {
 	u32 denied, audited;
 
+	if (avd->flags & AVD_FLAGS_NEVERAUDIT)
+		return 0;
+
 	denied = requested & ~avd->allowed;
 	if (unlikely(denied)) {
 		audited = denied & avd->auditdeny;
@@ -411,8 +414,8 @@ static inline u32 avc_xperms_audit_required(u32 requested,
 }
 
 static inline int avc_xperms_audit(u32 ssid, u32 tsid, u16 tclass,
-				   u32 requested, struct av_decision *avd,
-				   struct extended_perms_decision *xpd,
+				   u32 requested, const struct av_decision *avd,
+				   const struct extended_perms_decision *xpd,
 				   u8 perm, int result,
 				   struct common_audit_data *ad)
 {
@@ -509,7 +512,8 @@ static struct avc_node *avc_alloc_node(void)
 	return node;
 }
 
-static void avc_node_populate(struct avc_node *node, u32 ssid, u32 tsid, u16 tclass, struct av_decision *avd)
+static void avc_node_populate(struct avc_node *node, u32 ssid, u32 tsid,
+			      u16 tclass, const struct av_decision *avd)
 {
 	node->ae.ssid = ssid;
 	node->ae.tsid = tsid;
@@ -603,7 +607,8 @@ static int avc_latest_notif_update(u32 seqno, int is_insert)
  * the access vectors into a cache entry.
  */
 static void avc_insert(u32 ssid, u32 tsid, u16 tclass,
-		       struct av_decision *avd, struct avc_xperms_node *xp_node)
+		       const struct av_decision *avd,
+		       const struct avc_xperms_node *xp_node)
 {
 	struct avc_node *pos, *node = NULL;
 	u32 hvalue;
@@ -828,7 +833,7 @@ int __init avc_add_callback(int (*callback)(u32 event), u32 events)
  */
 static int avc_update_node(u32 event, u32 perms, u8 driver, u8 base_perm,
 			   u8 xperm, u32 ssid, u32 tsid, u16 tclass, u32 seqno,
-			   struct extended_perms_decision *xpd, u32 flags)
+			   const struct extended_perms_decision *xpd, u32 flags)
 {
 	u32 hvalue;
 	int rc = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bits`
- 外部函数：`unlikely`
- 大写宏：`NULL`
- 外部类型：`av_decision`
- 外部类型：`common_audit_data`
- 外部类型：`extended_perms`
- 外部类型：`extended_perms_decision`
- 外部类型：`hlist_node`
- 外部类型：`list_head`
- 外部类型：`rcu_head`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-dd7b4a120c` → 本草稿移入 `cases/defect/auto-linux-dd7b4a120c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
