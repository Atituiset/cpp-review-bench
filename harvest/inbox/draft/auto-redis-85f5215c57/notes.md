# auto-redis-85f5215c57

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #13637 (https://github.com/redis/redis/pull/13637)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 13（原始 PR diff 行 1073；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

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

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-85f5215c57` → 本草稿移入 `cases/defect/auto-redis-85f5215c57/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
