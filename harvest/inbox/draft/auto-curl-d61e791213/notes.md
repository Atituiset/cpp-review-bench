# auto-curl-d61e791213

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #3693 (https://github.com/curl/curl/pull/3693)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 847；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -838,15 +838,15 @@ int cert_stuff(struct connectdata *conn,
       return 0;
     }
 
-    file_type = do_file_type(key_type);
+    if(!key_file)
+      key_file = cert_file;
+    else
+      file_type = do_file_type(key_type);
 
     switch(file_type) {
     case SSL_FILETYPE_PEM:
       if(cert_done)
         break;
-      if(!key_file)
-        /* cert & key can only be in PEM case in the same file */
-        key_file = cert_file;
       /* FALLTHROUGH */
     case SSL_FILETYPE_ASN1:
       if(SSL_CTX_use_PrivateKey_file(ctx, key_file, file_type) != 1) {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-d61e791213` → 本草稿移入 `cases/defect/auto-curl-d61e791213/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
