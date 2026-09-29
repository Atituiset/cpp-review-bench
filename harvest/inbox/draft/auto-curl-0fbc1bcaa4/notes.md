# auto-curl-0fbc1bcaa4

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #16358 (https://github.com/curl/curl/pull/16358)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 1148；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1124,43 +1124,46 @@ void Curl_hostcache_clean(struct Curl_easy *data,
 CURLcode Curl_loadhostpairs(struct Curl_easy *data)
 {
   struct curl_slist *hostp;
-  const char *host_end;
 
   /* Default is no wildcard found */
   data->state.wildcard_resolve = FALSE;
 
   for(hostp = data->state.resolve; hostp; hostp = hostp->next) {
     char entry_id[MAX_HOSTCACHE_LEN];
-    if(!hostp->data)
+    const char *host = hostp->data;
+    struct Curl_str source;
+    if(!host)
       continue;
-    if(hostp->data[0] == '-') {
+    if(*host == '-') {
       curl_off_t num = 0;
       size_t entry_len;
-      size_t hlen = 0;
-      host_end = strchr(&hostp->data[1], ':');
-
-      if(host_end) {
-        hlen = host_end - &hostp->data[1];
-        host_end++;
-        if(Curl_str_number(&host_end, &num, 0xffff))
-          host_end = NULL;
+      host++;
+      if(!Curl_str_single(&host, '[')) {
+        if(Curl_str_until(&host, &source, MAX_IPADR_LEN, ']') ||
+           Curl_str_single(&host, ']') ||
+           Curl_str_single(&host, ':'))
+          continue;
       }
-      if(!host_end) {
-        infof(data, "Bad syntax CURLOPT_RESOLVE removal entry '%s'",
-              hostp->data);
-        continue;
+      else {
+        if(Curl_str_until(&host, &source, 4096, ':') ||
+           Curl_str_single(&host, ':')) {
+          continue;
+        }
       }
-      /* Create an entry id, based upon the hostname and port */
-      entry_len = create_hostcache_id(&hostp->data[1], hlen, (int)num,
-                                      entry_id, sizeof(entry_id));
-      if(data->share)
-        Curl_share_lock(data, CURL_LOCK_DATA_DNS, CURL_LOCK_ACCESS_SINGLE);
 
-      /* delete entry, ignore if it did not exist */
-      Curl_hash_delete(data->dns.hostcache, entry_id, entry_len + 1);
+      if(!Curl_str_number(&host, &num, 0xffff)) {
+        /* Create an entry id, based upon the hostname and port */
+        entry_len = create_hostcache_id(source.str, source.len, (int)num,
+                                        entry_id, sizeof(entry_id));
+        if(data->share)
+          Curl_share_lock(data, CURL_LOCK_DATA_DNS, CURL_LOCK_ACCESS_SINGLE);
 
-      if(data->share)
-        Curl_share_unlock(data, CURL_LOCK_DATA_DNS);
+        /* delete entry, ignore if it did not exist */
+        Curl_hash_delete(data->dns.hostcache, entry_id, entry_len + 1);
+
+        if(data->share)
+          Curl_share_unlock(data, CURL_LOCK_DATA_DNS);
+      }
     }
     else {
       struct Curl_dns_entry *dns;
@@ -1170,71 +1173,66 @@ CURLcode Curl_loadhostpairs(struct Curl_easy *data)
 #if !defined(CURL_DISABLE_VERBOSE_STRINGS)
       const char *addresses = NULL;
 #endif
-      const char *addr_begin;
-      const char *addr_end;
-      const char *port_ptr;
       curl_off_t port = 0;
-      const char *end_ptr;
       bool permanent = TRUE;
       bool error = TRUE;
-      char *host_begin = hostp->data;
-      size_t hlen = 0;
 
-      if(host_begin[0] == '+') {
-        host_begin++;
+      if(*host == '+') {
+        host++;
         permanent = FALSE;
       }
-      host_end = strchr(host_begin, ':');
-      if(!host_end)
-        goto err;
-      hlen = host_end - host_begin;
-
-      port_ptr = host_end + 1;
-      if(Curl_str_number(&port_ptr, &port, 0xffff) ||
-         (*port_ptr != ':'))
+      if(!Curl_str_single(&host, '[')) {
+        if(Curl_str_until(&host, &source, MAX_IPADR_LEN, ']') ||
+           Curl_str_single(&host, ']'))
+          continue;
+      }
+      else {
+        if(Curl_str_until(&host, &source, 4096, ':'))
+          continue;
+      }
+      if(Curl_str_single(&host, ':') ||
+         Curl_str_number(&host, &port, 0xffff) ||
+         Curl_str_single(&host, ':'))
         goto err;
-      end_ptr = port_ptr;
 
 #if !defined(CURL_DISABLE_VERBOSE_STRINGS)
-      addresses = end_ptr + 1;
+      addresses = host;
 #endif
 
-      while(*end_ptr) {
-        size_t alen;
+      /* start the address section */
+      
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-0fbc1bcaa4` → 本草稿移入 `cases/defect/auto-curl-0fbc1bcaa4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
