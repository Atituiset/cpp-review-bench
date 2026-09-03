# auto-redis-85f5215c57

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#13637](https://github.com/redis/redis/pull/13637) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #13637 (https://github.com/redis/redis/pull/13637)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1073；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 13637 Release 7.4.2 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1061,19 +1061,24 @@ int ACLSetSelector(aclSelector *selector, const char* op, size_t oplen) {
         int flags = 0;
         size_t offset = 1;
         if (op[0] == '%') {
+            int perm_ok = 1;
             for (; offset < oplen; offset++) {
                 if (toupper(op[offset]) == 'R' && !(flags & ACL_READ_PERMISSION)) {
                     flags |= ACL_READ_PERMISSION;
                 } else if (toupper(op[offset]) == 'W' && !(flags & ACL_WRITE_PERMISSION)) {
                     flags |= ACL_WRITE_PERMISSION;
-                } else if (op[offset] == '~' && flags) {
+                } else if (op[offset] == '~') {
                     offset++;
                     break;
                 } else {
-                    errno = EINVAL;
-                    return C_ERR;
+                    perm_ok = 0;
+                    break;
                 }
             }
+            if (!flags || !perm_ok) {
+                errno = EINVAL;
+                return C_ERR;
+            }
         } else {
             flags = ACL_ALL_PERMISSION;
         }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`toupper`
- 大写宏：`ACL_ALL_PERMISSION`
- 大写宏：`ACL_READ_PERMISSION`
- 大写宏：`ACL_WRITE_PERMISSION`
- 大写宏：`C_ERR`
- 大写宏：`EINVAL`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-85f5215c57` → 本草稿移入 `cases/defect/auto-redis-85f5215c57/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
