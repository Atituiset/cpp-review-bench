# auto-redis-97155c81e0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#13637](https://github.com/redis/redis/pull/13637) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #13637 (https://github.com/redis/redis/pull/13637)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 13637 Release 7.4.2 :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1,3 +1,16 @@
+/*
+ * Copyright (c) 2009-Present, Redis Ltd.
+ * All rights reserved.
+ *
+ * Copyright (c) 2024-present, Valkey contributors.
+ * All rights reserved.
+ *
+ * Licensed under your choice of the Redis Source Available License 2.0
+ * (RSALv2) or the Server Side Public License v1 (SSPLv1).
+ *
+ * Portions of this file are available under BSD3 terms; see REDISCONTRIBUTIONS for more information.
+ */
+
 #ifndef CLUSTER_LEGACY_H
 #define CLUSTER_LEGACY_H
 
@@ -51,6 +64,7 @@ typedef struct clusterLink {
 #define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node */
 #define CLUSTER_NODE_MIGRATE_TO 256 /* Master eligible for replica migration. */
 #define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failover. */
+#define CLUSTER_NODE_EXTENSIONS_SUPPORTED 1024 /* This node supports extensions. */
 #define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
 
 #define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
@@ -59,6 +73,7 @@ typedef struct clusterLink {
 #define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
 #define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
 #define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)
+#define nodeSupportsExtensions(n) ((n)->flags & CLUSTER_NODE_EXTENSIONS_SUPPORTED)
 
 /* This structure represent elements of node->fail_reports. */
 typedef struct clusterNodeFailReport {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`nodeCantFailover`
- 外部函数：`nodeFailed`
- 外部函数：`nodeIsSlave`
- 外部函数：`nodeTimedOut`
- 大写宏：`MEET`
- 外部类型：`Failure`
- 外部类型：`Master`
- 外部类型：`Need`
- 外部类型：`Send`
- 外部类型：`Slave`
- 外部类型：`The`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-97155c81e0` → 本草稿移入 `cases/defect/auto-redis-97155c81e0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
