# auto-curl-91448e0717

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #2144 (https://github.com/curl/curl/pull/2144)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 44（原始 PR diff 行 998；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -896,7 +896,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
         Curl_safefree(sshc->quote_path2);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "Attempt to set SFTP stats failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -916,7 +915,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
         Curl_safefree(sshc->quote_path2);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "symlink command failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -932,7 +930,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
                       (mode_t)data->set.new_directory_perms);
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "mkdir command failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -949,7 +946,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
         Curl_safefree(sshc->quote_path2);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "rename command failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -964,7 +960,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       rc = sftp_rmdir(sshc->sftp_session, sshc->quote_path1);
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "rmdir command failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -979,7 +974,6 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       rc = sftp_unlink(sshc->sftp_session, sshc->quote_path1);
       if(rc != 0 && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
-        err = sftp_get_error(sshc->sftp_session);
         failf(data, "rm command failed: %s",
               ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
@@ -995,16 +989,13 @@ static CURLcode myssh_statemach_act(struct connectdata *conn, bool *block)
       sftp_statvfs_t statvfs;
 
       statvfs = sftp_statvfs(sshc->sftp_session, sshc->quote_path1);
-      if(statvfs != 0 && !sshc->acceptfail) {
+      if(!statvfs && !sshc->acceptfail) {
         Curl_safefree(sshc->quote_path1);
-        err = sftp_get_error(sshc->sftp_session);
-        failf(data, "statvfs command failed: %s (%d)",
-              ssh_get_error(sshc->ssh_session), err);
+        failf(data, "statvfs command failed: %s",
+              ssh_get_error(sshc->ssh_session));
         state(conn, SSH_SFTP_CLOSE);
         sshc->nextstate = SSH_NO_STATE;
         sshc->actualcode = CURLE_QUOTE_ERROR;
-        if(statvfs)
-          sftp_statvfs_free(statvfs);
         break;
       }
       else {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-91448e0717` → 本草稿移入 `cases/defect/auto-curl-91448e0717/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
