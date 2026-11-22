# auto-linux-bd1bbf55ad

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#eef2bc5a82c25d25c3082f5d86d97a300caeb678](https://github.com/torvalds/linux/commit/eef2bc5a82c25d25c3082f5d86d97a300caeb678) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eef2bc5a82c25d25c3082f5d86d97a300caeb678 (https://github.com/torvalds/linux/commit/eef2bc5a82c25d25c3082f5d86d97a300caeb678)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 39；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eef2bc5a82c25d25c3082f5d86d97a300caeb678 Merge tag 'landlock-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/mic/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -21,6 +21,7 @@
 #include <net/af_unix.h>
 
 enum landlock_request_type;
+struct cred;
 struct dentry;
 struct landlock_blockers;
 struct landlock_domain;
@@ -30,13 +31,15 @@ struct landlock_ruleset;
 struct path;
 struct sock;
 struct task_struct;
+struct user_namespace;
 
 static_assert(sizeof(access_mask_t) <= sizeof(u64));
 
 TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_FS_CHANGE_TOPOLOGY);
 TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_FS_ACCESS);
 TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NET_ACCESS);
-TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NAMESPACE);
+TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NAMESPACE_CREATE);
+TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_NAMESPACE_ENTER);
 TRACE_DEFINE_ENUM(LANDLOCK_REQUEST_CAPABILITY);
 
 #ifdef CREATE_TRACE_POINTS
@@ -990,7 +993,8 @@ TRACE_EVENT(landlock_deny_access_net,
  * @namespace_id: Namespace ID, not directly visible from user space when
  *                creation was denied.
  *
- * Emitted when a Landlock domain denies namespace use.
+ * Emitted when a Landlock domain denies namespace creation or entry.  The
+ * namespace_op field distinguishes these operations.
  */
 TRACE_EVENT(landlock_deny_permission_namespace,
 
@@ -1021,13 +1025,18 @@ TRACE_EVENT(landlock_deny_permission_namespace,
 		__entry->namespace_id	= namespace_id;
 	),
 
-	TP_printk("domain=%llx same_exec=%d logged=%d blockers=%s namespace_type=0x%x namespace_id=%llu",
-		__entry->domain_id, __entry->same_exec, __entry->logged,
-		__entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE ?
-			__print_flags(__entry->blockers_access, "|",
-				      _LANDLOCK_PERMISSION_BLOCKER_NAMES) :
-			"unknown",
-		__entry->namespace_type, __entry->namespace_id)
+	TP_printk("domain=%llx same_exec=%d logged=%d blockers=%s namespace_op=%s namespace_type=0x%x namespace_id=%llu",
+		  __entry->domain_id, __entry->same_exec, __entry->logged,
+		  (__entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE_CREATE ||
+		   __entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE_ENTER) ?
+			  __print_flags(__entry->blockers_access, "|",
+					_LANDLOCK_PERMISSION_BLOCKER_NAMES) :
+			  "unknown",
+		  __entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE_CREATE ?
+			  "create" :
+		  __entry->blockers_type == LANDLOCK_REQUEST_NAMESPACE_ENTER ?
+			  "enter" : "unknown",
+		  __entry->namespace_type, __entry->namespace_id)
 );
 
 /**
@@ -1039,6 +1048,9 @@ TRACE_EVENT(landlock_deny_permission_namespace,
  * @logged: Whether this denial was selected for audit logging.
  * @blockers: Request type and final missing permission subset (never NULL).
  * @capability: CAP_* number that was denied.
+ * @cred: Credentials used by commoncap's capability check (never NULL).
+ * @user_ns: Target user namespace used by commoncap's capability check (never
+ *           NULL).
  *
  * Emitted when a Landlock domain denies capability use, except for checks
  * made with CAP_OPT_NOAUDIT, which are denied without emitting this event.
@@ -1047,9 +1059,11 @@ TRACE_EVENT(landlock_deny_permission_capability,
 
 	TP_PROTO(const struct landlock_hierarchy *hierarchy, bool same_exec,
 		 bool logged, const struct landlock_blockers *blockers,
-		 int capability),
+		 int capability, const struct cred *cred,
+		 const struct user_namespace *user_ns),
 
-	TP_ARGS(hierarchy, same_exec, logged, blockers, capability),
+	TP_ARGS(hierarchy, same_exec, logged, blockers, capability, cred,
+		user_ns),
 
 	TP_STRUCT__entry(
 		__field(	u64,		domain_id	)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__field`
- 外部函数：`__print_flags`
- 外部函数：`subset`
- 大写宏：`CAP_`
- 大写宏：`CAP_OPT_NOAUDIT`
- 大写宏：`CREATE_TRACE_POINTS`
- 大写宏：`LANDLOCK_REQUEST_CAPABILITY`
- 大写宏：`LANDLOCK_REQUEST_FS_ACCESS`
- 大写宏：`LANDLOCK_REQUEST_FS_CHANGE_TOPOLOGY`
- 大写宏：`LANDLOCK_REQUEST_NAMESPACE`
- 大写宏：`LANDLOCK_REQUEST_NET_ACCESS`
- 大写宏：`NULL`
- 大写宏：`TP_ARGS`
- 大写宏：`TP_PROTO`
- 大写宏：`TRACE_DEFINE_ENUM`
- 大写宏：`TRACE_EVENT`
- 外部类型：`Emitted`
- 外部类型：`Landlock`
- 外部类型：`Namespace`
- 外部类型：`Request`
- 外部类型：`Whether`
- 外部类型：`access_mask_t`
- 外部类型：`dentry`
- 外部类型：`landlock_blockers`
- 外部类型：`landlock_domain`
- 外部类型：`landlock_hierarchy`
- 外部类型：`landlock_request_type`
- 外部类型：`path`
- 外部类型：`sock`
- 外部类型：`task_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-bd1bbf55ad` → 本草稿移入 `cases/defect/auto-linux-bd1bbf55ad/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
