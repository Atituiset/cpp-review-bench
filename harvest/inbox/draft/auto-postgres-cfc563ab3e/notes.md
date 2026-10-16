# auto-postgres-cfc563ab3e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a39e53a700c5b728f2f7dab964e33b157979fb3a](https://github.com/postgres/postgres/commit/a39e53a700c5b728f2f7dab964e33b157979fb3a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a39e53a700c5b728f2f7dab964e33b157979fb3a (https://github.com/postgres/postgres/commit/a39e53a700c5b728f2f7dab964e33b157979fb3a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3903；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a39e53a700c5b728f2f7dab964e33b157979fb3a Fix DROP DATABASE FORCE failing on parallel autovacuum workers. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3882,8 +3882,9 @@ TerminateOtherDBBackends(Oid databaseId)
 		 * Permissions checks relax the pg_terminate_backend checks in two
 		 * ways, both by omitting the !OidIsValid(proc->roleId) check:
 		 *
-		 * - Accept terminating autovacuum workers, since DROP DATABASE
-		 * without FORCE terminates them.
+		 * - Accept terminating autovacuum workers and the parallel workers
+		 * launched by them, since DROP DATABASE without FORCE terminates
+		 * them.
 		 *
 		 * - Accept terminating bgworkers.  For bgworker authors, it's
 		 * convenient to be able to recommend FORCE if a worker is blocking
@@ -3900,14 +3901,30 @@ TerminateOtherDBBackends(Oid databaseId)
 
 			if (proc != NULL)
 			{
-				if (superuser_arg(proc->roleId) && !superuser())
+				PGPROC	   *leader = proc->lockGroupLeader;
+				Oid			roleId = proc->roleId;
+
+				/*
+				 * An autovacuum worker and a bgworker with no user name
+				 * advertise no role in PGPROC and run as the bootstrap
+				 * superuser (see InitializeSessionUserIdStandalone()
+				 * callsites in postinit.c). The parallel workers they launch
+				 * inherit the same superuser and advertise it in their
+				 * PGPROC. Accept such parallel workers for terminating as
+				 * well.
+				 */
+				if (leader != NULL && leader != proc &&
+					!OidIsValid(leader->roleId))
+					roleId = leader->roleId;
+
+				if (superuser_arg(roleId) && !superuser())
 					ereport(ERROR,
 							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
 							 errmsg("permission denied to terminate process"),
 							 errdetail("Only roles with the %s attribute may terminate processes of roles with the %s attribute.",
 									   "SUPERUSER", "SUPERUSER")));
 
-				if (!has_privs_of_role(GetUserId(), proc->roleId) &&
+				if (!has_privs_of_role(GetUserId(), roleId) &&
 					!has_privs_of_role(GetUserId(), ROLE_PG_SIGNAL_BACKEND))
 					ereport(ERROR,
 							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`has_privs_of_role`
- 外部函数：`superuser`
- 外部函数：`superuser_arg`
- 大写宏：`DATABASE`
- 大写宏：`DROP`
- 大写宏：`ERRCODE_INSUFFICIENT_PRIVILEGE`
- 大写宏：`ERROR`
- 大写宏：`FORCE`
- 大写宏：`NULL`
- 大写宏：`ROLE_PG_SIGNAL_BACKEND`
- 大写宏：`SUPERUSER`
- 外部类型：`Accept`
- 外部类型：`For`
- 外部类型：`Only`
- 外部类型：`Permissions`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cfc563ab3e` → 本草稿移入 `cases/defect/auto-postgres-cfc563ab3e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
