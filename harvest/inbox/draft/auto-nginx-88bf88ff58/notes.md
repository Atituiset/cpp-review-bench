# auto-nginx-88bf88ff58

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1598 (https://github.com/nginx/nginx/pull/1598)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -146,6 +146,17 @@ ngx_select_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
         return NGX_ERROR;
     }
 
+    /* disable warning: the default FD_SETSIZE is 1024U in FreeBSD 5.x-10.x */
+
+    if ((event == NGX_READ_EVENT || event == NGX_WRITE_EVENT)
+        && (unsigned) c->fd >= FD_SETSIZE)
+    {
+        ngx_log_error(NGX_LOG_ERR, ev->log, 0,
+                      "maximum number of descriptors "
+                      "supported by select() is %ud", FD_SETSIZE);
+        return NGX_ERROR;
+    }
+
     if (event == NGX_READ_EVENT) {
         FD_SET(c->fd, &master_read_fd_set);
 
@@ -411,8 +422,6 @@ ngx_select_init_conf(ngx_cycle_t *cycle, void *conf)
         return NGX_CONF_OK;
     }
 
-    /* disable warning: the default FD_SETSIZE is 1024U in FreeBSD 5.x */
-
     if (cycle->connection_n > FD_SETSIZE) {
         ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                       "the maximum number of files "
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-88bf88ff58` → 本草稿移入 `cases/defect/auto-nginx-88bf88ff58/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
