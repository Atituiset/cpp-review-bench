# auto-vim-6d766f2367

## 来源（采集溯源）
- 来源仓: vim/vim
- 源 PR: #13354 (https://github.com/vim/vim/pull/13354)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3562；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -3559,4 +3559,4 @@ EXTERN char e_aptypes_is_null_nr_str[]
 EXTERN char e_xattr_e2big[]
 	INIT(= N_("E1508: Size of the extended attribute value is larger than the maximum size allowed"));
 EXTERN char e_xattr_other[]
-	INIT(= N_("E1509: Error occured when reading or writing extended attribute"));
+	INIT(= N_("E1509: Error occurred when reading or writing extended attribute"));
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-vim-6d766f2367` → 本草稿移入 `cases/defect/auto-vim-6d766f2367/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
