# auto-vim-f347faf33d

## 来源（采集溯源）
- 来源仓: vim/vim
- 源 PR: #12749 (https://github.com/vim/vim/pull/12749)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1117；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1114,7 +1114,7 @@ ex_messages(exarg_T *eap)
 	    msg_attr(
 		    // Translator: Please replace the name and email address
 		    // with the appropriate text for your translation.
-		    _("Messages maintainer: Bram Moolenaar <Bram@vim.org>"),
+		    _("Messages maintainer: The Vim Project"),
 		    HL_ATTR(HLF_T));
     }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-vim-f347faf33d` → 本草稿移入 `cases/defect/auto-vim-f347faf33d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
