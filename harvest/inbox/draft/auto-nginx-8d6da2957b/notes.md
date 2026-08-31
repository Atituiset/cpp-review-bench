# auto-nginx-8d6da2957b

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1662 (https://github.com/nginx/nginx/pull/1662)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 155；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -134,6 +134,13 @@ ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size, ngx_log_t *log)
         }
 
         ngx_log_error(NGX_LOG_ALERT, log, err, "recvmsg() failed");
+
+        if (err == NGX_EMSGSIZE || err == NGX_EMFILE) {
+            /* file descriptor table is full */
+            ngx_memzero(ch, size);
+            return 0;
+        }
+
         return NGX_ERROR;
     }
 
@@ -152,27 +159,29 @@ ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size, ngx_log_t *log)
 
     if (ch->command == NGX_CMD_OPEN_CHANNEL) {
 
-        if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
+        if (msg.msg_controllen < (socklen_t) CMSG_LEN(sizeof(int))) {
             ngx_log_error(NGX_LOG_ALERT, log, 0,
                           "recvmsg() returned too small ancillary data");
-            return NGX_ERROR;
-        }
+            ch->fd = -1;
 
-        if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
+        } else if (cmsg.cm.cmsg_level != SOL_SOCKET
+                   || cmsg.cm.cmsg_type != SCM_RIGHTS)
         {
             ngx_log_error(NGX_LOG_ALERT, log, 0,
                           "recvmsg() returned invalid ancillary data "
                           "level %d or type %d",
                           cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
             return NGX_ERROR;
-        }
 
-        /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
+        } else {
 
-        ngx_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
+            /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
+
+            ngx_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
+        }
     }
 
-    if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {
+    if (msg.msg_flags & MSG_TRUNC) {
         ngx_log_error(NGX_LOG_ALERT, log, 0,
                       "recvmsg() truncated data");
     }
@@ -183,10 +192,11 @@ ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size, ngx_log_t *log)
         if (msg.msg_accrightslen != sizeof(int)) {
             ngx_log_error(NGX_LOG_ALERT, log, 0,
                           "recvmsg() returned no ancillary data");
-            return NGX_ERROR;
-        }
+            ch->fd = -1;
 
-        ch->fd = fd;
+        } else {
+            ch->fd = fd;
+        }
     }
 
 #endif
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-8d6da2957b` → 本草稿移入 `cases/defect/auto-nginx-8d6da2957b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
