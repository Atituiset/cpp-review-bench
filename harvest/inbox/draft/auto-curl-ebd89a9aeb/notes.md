# auto-curl-ebd89a9aeb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#15289](https://github.com/curl/curl/pull/15289) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 354 |
| 编译错误数（gcc syntax-only） | 80（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15289 (https://github.com/curl/curl/pull/15289)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 893；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15289 lib: fix function pointers to please UndefinedBehaviorSanitizer :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -451,7 +451,7 @@ struct Curl_multi *Curl_multi_handle(size_t hashsize, /* socket hash */
   return NULL;
 }
 
-struct Curl_multi *curl_multi_init(void)
+CURLM *curl_multi_init(void)
 {
   return Curl_multi_handle(CURL_SOCKET_HASH_TABLE_SIZE,
                            CURL_CONNECTION_HASH_SIZE,
@@ -472,10 +472,11 @@ static void multi_warn_debug(struct Curl_multi *multi, struct Curl_easy *data)
 #define multi_warn_debug(x,y) Curl_nop_stmt
 #endif
 
-CURLMcode curl_multi_add_handle(struct Curl_multi *multi,
-                                struct Curl_easy *data)
+CURLMcode curl_multi_add_handle(CURLM *m, CURL *d)
 {
   CURLMcode rc;
+  struct Curl_multi *multi = m;
+  struct Curl_easy *data = d;
   /* First, make some basic checks that the CURLM handle is a good handle */
   if(!GOOD_MULTI_HANDLE(multi))
     return CURLM_BAD_HANDLE;
@@ -772,10 +773,10 @@ static void close_connect_only(struct connectdata *conn,
     connclose(conn, "Removing connect-only easy handle");
 }
 
-CURLMcode curl_multi_remove_handle(struct Curl_multi *multi,
-                                   struct Curl_easy *data)
+CURLMcode curl_multi_remove_handle(CURLM *m, CURL *d)
 {
-  struct Curl_easy *easy = data;
+  struct Curl_multi *multi = m;
+  struct Curl_easy *data = d;
   bool premature;
   struct Curl_llist_node *e;
   CURLMcode rc;
@@ -850,7 +851,7 @@ CURLMcode curl_multi_remove_handle(struct Curl_multi *multi,
 
   /* This ignores the return code even in case of problems because there is
      nothing more to do about that, here */
-  (void)singlesocket(multi, easy); /* to let the application know what sockets
+  (void)singlesocket(multi, data); /* to let the application know what sockets
                                       that vanish with this handle */
 
   /* Remove the association between the connection and the handle */
@@ -890,7 +891,7 @@ CURLMcode curl_multi_remove_handle(struct Curl_multi *multi,
   for(e = Curl_llist_head(&multi->msglist); e; e = Curl_node_next(e)) {
     struct Curl_message *msg = Curl_node_elem(e);
 
-    if(msg->extmsg.easy_handle == easy) {
+    if(msg->extmsg.easy_handle == data) {
       Curl_node_remove(e);
       /* there can only be one from this specific handle */
       break;
@@ -1141,7 +1142,7 @@ static void multi_getsock(struct Curl_easy *data,
   }
 }
 
-CURLMcode curl_multi_fdset(struct Curl_multi *multi,
+CURLMcode curl_multi_fdset(CURLM *m,
                            fd_set *read_fd_set, fd_set *write_fd_set,
                            fd_set *exc_fd_set, int *max_fd)
 {
@@ -1150,6 +1151,7 @@ CURLMcode curl_multi_fdset(struct Curl_multi *multi,
      and then we must make sure that is done. */
   int this_max_fd = -1;
   struct Curl_llist_node *e;
+  struct Curl_multi *multi = m;
   (void)exc_fd_set; /* not used */
 
   if(!GOOD_MULTI_HANDLE(multi))
@@ -1182,14 +1184,15 @@ CURLMcode curl_multi_fdset(struct Curl_multi *multi,
   return CURLM_OK;
 }
 
-CURLMcode curl_multi_waitfds(struct Curl_multi *multi,
+CURLMcode curl_multi_waitfds(CURLM *m,
                              struct curl_waitfd *ufds,
                              unsigned int size,
                              unsigned int *fd_count)
 {
   struct curl_waitfds cwfds;
   CURLMcode result = CURLM_OK;
   struct Curl_llist_node *e;
+  struct Curl_multi *multi = m;
 
   if(!ufds)
     return CURLM_BAD_FUNCTION_ARGUMENT;
@@ -1487,7 +1490,7 @@ static CURLMcode multi_wait(struct Curl_multi *multi,
   return result;
 }
 
-CURLMcode curl_multi_wait(struct Curl_multi *multi,
+CURLMcode curl_multi_wait(CURLM *multi,
                           struct curl_waitfd extra_fds[],
                           unsigned int extra_nfds,
                           int timeout_ms,
@@ -1497,7 +1500,7 @@ CURLMcode curl_multi_wait(struct Curl_multi *multi,
                     FALSE);
 }
 
-CURLMcode curl_multi_poll(struct Curl_multi *multi,
+CURLMcode curl_multi_poll(CURLM *multi,
                           struct curl_waitfd extra_fds[],
                  
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`above`
- 外部函数：`atoi`
- 外部函数：`calloc`
- 外部函数：`connclose`
- 外部函数：`connect`
- 外部函数：`connect_it`
- 外部函数：`connecting`
- 外部函数：`connkeep`
- 外部函数：`curl_easy_perform`
- 外部函数：`curl_multi_add_handle`
- 外部函数：`curl_url`
- 外部函数：`curl_url_cleanup`
- 外部函数：`curl_url_get`
- 外部函数：`curl_url_set`
- 外部函数：`curl_url_strerror`
- 外部函数：`do_it`
- 外部函数：`do_more`
- 外部函数：`doing`
- 外部函数：`done`
- 外部函数：`failf`
- 外部函数：`fmultidone`
- 外部函数：`fprereq`
- 外部函数：`fprintf`
- 外部函数：`free`
- 外部函数：`infof`
- 外部函数：`malloc`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`multi_follow`
- 外部函数：`multi_ischanged`
- 外部函数：`multi_wait`
- 外部函数：`multistate`
- 外部函数：`one`
- 外部函数：`perform`
- 外部函数：`proto_getsock`
- 外部函数：`resources`
- 外部函数：`sh_freeentry`
- 外部函数：`signal`
- 外部函数：`sigpipe_apply`
- 外部函数：`sigpipe_init`
- 外部函数：`sigpipe_restore`
- 外部函数：`socket`
- 外部函数：`socket_cb`
- 外部函数：`strdup`
- 外部函数：`streamclose`
- 外部函数：`that`
- 外部函数：`timer_cb`
- 大写宏：`API`
- 大写宏：`BAD`
- 大写宏：`BIT`
- 大写宏：`COMPLETED`
- 大写宏：`CONNECT`
- 大写宏：`CONNECTING`
- 大写宏：`CONNECT_ONLY`
- 大写宏：`CONNECT_PEND`
- 大写宏：`CONN_INUSE`
- 大写宏：`CURLEASY_MAGIC_NUMBER`
- 大写宏：`CURLE_ABORTED_BY_CALLBACK`
- 大写宏：`CURLE_GOT_NOTHING`
- 大写宏：`CURLE_HTTP2_STREAM`
- 大写宏：`CURLE_NO_CONNECTION_AVAILABLE`
- 大写宏：`CURLE_OK`
- 大写宏：`CURLE_OPERATION_TIMEDOUT`
- 大写宏：`CURLE_OUT_OF_MEMORY`
- 大写宏：`CURLE_READ_ERROR`
- 大写宏：`CURLE_RECV_ERROR`
- 大写宏：`CURLE_SEND_ERROR`
- 大写宏：`CURLE_TOO_MANY_REDIRECTS`
- 大写宏：`CURLE_WRITE_ERROR`
- 大写宏：`CURLM`
- 大写宏：`CURLMSG_DONE`
- 大写宏：`CURLM_ABORTED_BY_CALLBACK`
- 大写宏：`CURLM_ADDED_ALREADY`
- 大写宏：`CURLM_BAD_EASY_HANDLE`
- 大写宏：`CURLM_BAD_FUNCTION_ARGUMENT`
- 大写宏：`CURLM_BAD_HANDLE`
- 大写宏：`CURLM_CALL_MULTI_PERFORM`
- 大写宏：`CURLM_INTERNAL_ERROR`
- 大写宏：`CURLM_OK`
- 大写宏：`CURLM_OUT_OF_MEMORY`
- 大写宏：`CURLM_RECURSIVE_API_CALL`
- 大写宏：`CURLOPT_POSTREDIR`
- 大写宏：`CURLRES_ASYNCH`
- 大写宏：`CURLU`
- 大写宏：`CURLUPART_FRAGMENT`
- 大写宏：`CURLUPART_PASSWORD`
- 大写宏：`CURLUPART_PORT`
- 大写宏：`CURLUPART_SCHEME`
- 大写宏：`CURLUPART_URL`
- 大写宏：`CURLUPART_USER`
- 大写宏：`CURLU_ALLOW_SPACE`
- 大写宏：`CURLU_DEFAULT_PORT`
- 大写宏：`CURLU_NON_SUPPORT_SCHEME`
- 大写宏：`CURLU_PATH_AS_IS`
- 大写宏：`CURLU_URLENCODE`
- 大写宏：`CURLWC_DONE`
- 大写宏：`CURLWC_SKIP`
- 大写宏：`CURL_DISABLE_FTP`
- 大写宏：`CURL_DISABLE_HTTP`
- 大写宏：`CURL_DISABLE_PROXY`
- 大写宏：`CURL_DISABLE_VERBOSE_STRINGS`
- 大写宏：`CURL_HTTP_VERSION_1_1`
- 大写宏：`CURL_LOCK_DATA_PSL`
- 大写宏：`CURL_POLL_IN`
- 大写宏：`CURL_POLL_OUT`
- 大写宏：`CURL_POLL_REMOVE`
- 大写宏：`CURL_PREREQFUNC_OK`
- 大写宏：`CURL_REDIR_POST_301`
- 大写宏：`CURL_REDIR_POST_302`
- 大写宏：`CURL_REDIR_POST_303`
- 大写宏：`CURL_SOCKET_BAD`
- 大写宏：`CURL_SOCKET_TIMEOUT`
- 大写宏：`CURL_WANT_RECV`
- 大写宏：`DEBUGASSERT`
- 大写宏：`DEBUGBUILD`
- 大写宏：`DEBUGF`
- 大写宏：`DID`
- 大写宏：`DNS`
- 大写宏：`DOING`
- 大写宏：`DOING_MORE`
- 大写宏：`DONE`
- 大写宏：`DO_DONE`
- 大写宏：`DO_MORE`
- 大写宏：`ENABLE_WAKEUP`
- 大写宏：`EXPIRE_CONNECTTIMEOUT`
- 大写宏：`EXPIRE_LAST`
- 大写宏：`EXPIRE_RUN_NOW`
- 大写宏：`EXPIRE_TIMEOUT`
- 大写宏：`EXPIRE_TOOFAST`
- 大写宏：`FAKE`
- 大写宏：`FALLTHROUGH`
- 大写宏：`FALSE`
- 大写宏：`FIRSTSOCKET`
- 大写宏：`FMT_OFF_T`
- 大写宏：`FMT_TIMEDIFF_T`
- 大写宏：`FOLLOW_FAKE`
- 大写宏：`FOLLOW_NONE`
- 大写宏：`FOLLOW_REDIR`
- 大写宏：`FOLLOW_RETRY`
- 大写宏：`FTP`
- 大写宏：`GET`
- 大写宏：`GETSOCK_BLANK`
- 大写宏：`GETSOCK_READSOCK`
- 大写宏：`GETSOCK_WRITESOCK`
- 大写宏：`GOOD_EASY_HANDLE`
- 大写宏：`GSS_AUTHRECV`
- 大写宏：`HAVE_MSG_NOSIGNAL`
- 大写宏：`HAVE_SIGNAL`
- 大写宏：`HCACHE_MULTI`
- 大写宏：`HCACHE_NONE`
- 大写宏：`HEAD`
- 大写宏：`HTTP`
- 大写宏：`HTTPREQ_GET`
- 大写宏：`HTTPREQ_POST`
- 大写宏：`HTTPREQ_POST_FORM`
- 大写宏：`HTTPREQ_POST_MIME`
- 大写宏：`HTTP_1_1_REQUIRED`
- 大写宏：`INIT`
- 大写宏：`MAY`
- 大写宏：`MORE`
- 大写宏：`MSGSENT`
- 大写宏：`MSTATE_COMPLETED`
- 大写宏：`MSTATE_CONNECT`
- 大写宏：`MSTATE_CONNECTING`
- 大写宏：`MSTATE_DID`
- 大写宏：`MSTATE_DO`
- 大写宏：`MSTATE_DOING`
- 大写宏：`MSTATE_DOING_MORE`
- 大写宏：`MSTATE_DONE`
- 大写宏：`MSTATE_INIT`
- 大写宏：`MSTATE_LAST`
- 大写宏：`MSTATE_MSGSENT`
- 大写宏：`MSTATE_PENDING`
- 大写宏：`MSTATE_PERFORMING`
- 大写宏：`MSTATE_PROTOCONNECT`
- 大写宏：`MSTATE_PROTOCONNECTING`
- 大写宏：`MSTATE_RATELIMITING`
- 大写宏：`MSTATE_RESOLVING`
- 大写宏：`MSTATE_SETUP`
- 大写宏：`MSTATE_TUNNELING`
- 大写宏：`MUST`
- 大写宏：`NEGOTIATE`
- 大写宏：`NOT`
- 大写宏：`NOTE`
- 大写宏：`NTLM`
- 大写宏：`NTLMSTATE_TYPE2`
- 大写宏：`NULL`
- 大写宏：`PENDING`
- 大写宏：`PERFORMING`
- 大写宏：`POLL`
- 大写宏：`POST`
- 大写宏：`PROTOCONNECT`
- 大写宏：`PROTOCONNECTING`
- 大写宏：`PROTOPT_DUAL`
- 大写宏：`PROTOPT_WILDCARD`
- 大写宏：`PSL`
- 大写宏：`RATELIMITING`
- 大写宏：`READ`
- 大写宏：`RESOLVING`
- 大写宏：`RFC1945`
- 大写宏：`RFC2616`
- 大写宏：`RFC7231`
- 大写宏：`SEND_ERROR`
- 大写宏：`SETUP`
- 大写宏：`SIGPIPE`
- 大写宏：`SIGPIPE_MEMBER`
- 大写宏：`SIGPIPE_VARIABLE`
- 大写宏：`STATE`
- 大写宏：`TCP`
- 大写宏：`TIMER_POSTQUEUE`
- 大写宏：`TIMER_PRETRANSFER`
- 大写宏：`TIMER_REDIRECT`
- 大写宏：`TIMER_STARTOP`
- 大写宏：`TIMER_STARTSINGLE`
- 大写宏：`TRUE`
- 大写宏：`TUNNELING`
- 大写宏：`URI`
- 大写宏：`URL`
- 大写宏：`USE_EVENTFD`
- 大写宏：`USE_LIBPSL`
- 大写宏：`USE_NTLM`
- 大写宏：`USE_SPNEGO`
- 大写宏：`USE_WINSOCK`
- 大写宏：`WAITCONNECT`
- 大写宏：`WAITDO`
- 大写宏：`WARNING`
- 大写宏：`WRITE`
- 大写宏：`WWW`
- 大写宏：`XXXX`
- 外部类型：`Aborted`
- 外部类型：`Act`
- 外部类型：`Add`
- 外部类型：`Analyzers`
- 外部类型：`Authenticate`
- 外部类型：`Authorization`
- 外部类型：`Basic`
- 外部类型：`CURLMcode`
- 外部类型：`CURLMoption`
- 外部类型：`CURLMsg`
- 外部类型：`CURLMstate`
- 外部类型：`CURLUcode`
- 外部类型：`CURLcode`
- 外部类型：`Call`
- 外部类型：`Check`
- 外部类型：`Choices`
- 外部类型：`Cleanup`
- 外部类型：`Clear`
- 外部类型：`Compare`
- 外部类型：`Connect`
- 外部类型：`Connection`
- 外部类型：`Curl_dns_entry`
- 外部类型：`Curl_easy`
- 外部类型：`Curl_handler`
- 外部类型：`Curl_hash`
- 外部类型：`Curl_hash_element`
- 外部类型：`Curl_hash_iterator`
- 外部类型：`Curl_init_CONNECT`
- 外部类型：`Curl_llist`
- 外部类型：`Curl_llist_node`
- 外部类型：`Curl_message`
- 外部类型：`Curl_multi`
- 外部类型：`Curl_nop_stmt`
- 外部类型：`Curl_resolv_getsock`
- 外部类型：`Curl_sh_entry`
- 外部类型：`Curl_tree`
- 外部类型：`Decrease`
- 外部类型：`Default`
- 外部类型：`Detect`
- 外部类型：`Disconnect`
- 外部类型：`Do`
- 外部类型：`Downgrades`
- 外部类型：`During`
- 外部类型：`Expire`
- 外部类型：`Fill`
- 外部类型：`First`
- 外部类型：`Follow`
- 外部类型：`For`
- 外部类型：`Force`
- 外部类型：`Found`
- 外部类型：`Handle`
- 外部类型：`Have`
- 外部类型：`Having`
- 外部类型：`Hostname`
- 外部类型：`If`
- 外部类型：`Important`
- 外部类型：`In`
- 外部类型：`Indicate`
- 外部类型：`Inform`
- 外部类型：`Initialize`
- 外部类型：`Insert`
- 外部类型：`Instead`
- 外部类型：`Internal`
- 外部类型：`Issue`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Let`
- 外部类型：`Location`
- 外部类型：`Make`
- 外部类型：`Many`
- 外部类型：`Modified`
- 外部类型：`Moved`
- 外部类型：`Multiple`
- 外部类型：`Need`
- 外部类型：`News`
- 外部类型：`No`
- 外部类型：`None`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Only`
- 外部类型：`Operation`
- 外部类型：`Other`
- 外部类型：`Our`
- 外部类型：`Perform`
- 外部类型：`Permanently`
- 外部类型：`Prevent`
- 外部类型：`Proxy`
- 外部类型：`Redirect`
- 外部类型：`Remember`
- 外部类型：`Remove`
- 外部类型：`Removing`
- 外部类型：`Resolving`
- 外部类型：`Running`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Setup`
- 外部类型：`Since`
- 外部类型：`SingleRequest`
- 外部类型：`Skip`
- 外部类型：`Socket`
- 外部类型：`Some`
- 外部类型：`Stop`
- 外部类型：`Switch`
- 外部类型：`Temporary`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Transfer`
- 外部类型：`Transitional`
- 外部类型：`Under`
- 外部类型：`Unless`
- 外部类型：`Unmatched`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`Verify`
- 外部类型：`Wait`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`WildcardData`
- 外部类型：`With`
- 外部类型：`connectdata`
- 外部类型：`curl_socket_t`
- 外部类型：`curl_waitfd`
- 外部类型：`curl_waitfds`
- 外部类型：`curltime`
- 外部类型：`easy_pollset`
- 外部类型：`is`
- 外部类型：`remaining`
- 外部类型：`size_t`
- 外部类型：`that`
- 外部类型：`time_node`
- 外部类型：`time_t`
- 外部类型：`timediff_t`
- 外部类型：`with`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-ebd89a9aeb` → 本草稿移入 `cases/defect/auto-curl-ebd89a9aeb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
