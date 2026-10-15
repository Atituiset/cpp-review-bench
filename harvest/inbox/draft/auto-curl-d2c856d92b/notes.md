# auto-curl-d2c856d92b

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7babac86904c7ac0b34a22240737c7261cb059eb (https://github.com/curl/curl/commit/7babac86904c7ac0b34a22240737c7261cb059eb)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 699；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -696,12 +696,9 @@ void restore_signal_handlers(bool keep_sigalrm)
       }
     }
   }
-  if(exit_event) {
-    if(CloseHandle(exit_event)) {
-      exit_event = NULL;
-    }
-  }
 #endif
+  if(exit_event && CloseHandle(exit_event))
+    exit_event = NULL;
 #endif
 }
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-d2c856d92b` → 本草稿移入 `cases/defect/auto-curl-d2c856d92b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
