# auto-curl-4785d72ce3

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #1727 (https://github.com/curl/curl/pull/1727)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 890；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -887,21 +887,24 @@ static CURLcode ftp_state_cwd(struct connectdata *conn)
        dir) this then allows for a second try to CWD to it */
     ftpc->count3 = (conn->data->set.ftp_create_missing_dirs==2)?1:0;
 
-    if(conn->bits.reuse && ftpc->entrypath) {
+    if((conn->data->set.ftp_filemethod == FTPFILE_NOCWD) && !ftpc->cwdcount)
+      /* No CWD necessary */
+      result = ftp_state_mdtm(conn);
+    else if(conn->bits.reuse && ftpc->entrypath) {
       /* This is a re-used connection. Since we change directory to where the
          transfer is taking place, we must first get back to the original dir
          where we ended up after login: */
-      ftpc->count1 = 0; /* we count this as the first path, then we add one
-                          for all upcoming ones in the ftp->dirs[] array */
+      ftpc->cwdcount = 0; /* we count this as the first path, then we add one
+                             for all upcoming ones in the ftp->dirs[] array */
       PPSENDF(&conn->proto.ftpc.pp, "CWD %s", ftpc->entrypath);
       state(conn, FTP_CWD);
     }
     else {
       if(ftpc->dirdepth) {
-        ftpc->count1 = 1;
+        ftpc->cwdcount = 1;
         /* issue the first CWD, the rest is sent when the CWD responses are
            received... */
-        PPSENDF(&conn->proto.ftpc.pp, "CWD %s", ftpc->dirs[ftpc->count1 -1]);
+        PPSENDF(&conn->proto.ftpc.pp, "CWD %s", ftpc->dirs[ftpc->cwdcount -1]);
         state(conn, FTP_CWD);
       }
       else {
@@ -2936,10 +2939,10 @@ static CURLcode ftp_statemach_act(struct connectdata *conn)
       if(ftpcode/100 != 2) {
         /* failure to CWD there */
         if(conn->data->set.ftp_create_missing_dirs &&
-           ftpc->count1 && !ftpc->count2) {
+           ftpc->cwdcount && !ftpc->count2) {
           /* try making it */
           ftpc->count2++; /* counter to prevent CWD-MKD loops */
-          PPSENDF(&ftpc->pp, "MKD %s", ftpc->dirs[ftpc->count1 - 1]);
+          PPSENDF(&ftpc->pp, "MKD %s", ftpc->dirs[ftpc->cwdcount - 1]);
           state(conn, FTP_MKD);
         }
         else {
@@ -2953,9 +2956,9 @@ static CURLcode ftp_statemach_act(struct connectdata *conn)
       else {
         /* success */
         ftpc->count2=0;
-        if(++ftpc->count1 <= ftpc->dirdepth) {
+        if(++ftpc->cwdcount <= ftpc->dirdepth) {
           /* send next CWD */
-          PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->count1 - 1]);
+          PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->cwdcount - 1]);
         }
         else {
           result = ftp_state_mdtm(conn);
@@ -2973,7 +2976,7 @@ static CURLcode ftp_statemach_act(struct connectdata *conn)
       }
       state(conn, FTP_CWD);
       /* send CWD */
-      PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->count1 - 1]);
+      PPSENDF(&ftpc->pp, "CWD %s", ftpc->dirs[ftpc->cwdcount - 1]);
       break;
 
     case FTP_MDTM:
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-4785d72ce3` → 本草稿移入 `cases/defect/auto-curl-4785d72ce3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
