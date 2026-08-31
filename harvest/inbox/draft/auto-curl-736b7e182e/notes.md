# auto-curl-736b7e182e

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #8131 (https://github.com/curl/curl/pull/8131)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 408；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -243,6 +243,26 @@ static void trhash_dtor(void *nada)
   (void)nada;
 }
 
+/*
+ * The sockhash has its own separate subhash in each entry that need to be
+ * safely destroyed first.
+ */
+static void sockhash_destroy(struct Curl_hash *h)
+{
+  struct Curl_hash_iterator iter;
+  struct Curl_hash_element *he;
+
+  DEBUGASSERT(h);
+  Curl_hash_start_iterate(h, &iter);
+  he = Curl_hash_next_element(&iter);
+  while(he) {
+    struct Curl_sh_entry *sh = (struct Curl_sh_entry *)he->ptr;
+    Curl_hash_destroy(&sh->transfers);
+    he = Curl_hash_next_element(&iter);
+  }
+  Curl_hash_destroy(h);
+}
+
 
 /* make sure this socket is present in the hash for this handle */
 static struct Curl_sh_entry *sh_addentry(struct Curl_hash *sh,
@@ -405,7 +425,7 @@ struct Curl_multi *Curl_multi_handle(int hashsize, /* socket hash */
 
   error:
 
-  Curl_hash_destroy(&multi->sockhash);
+  sockhash_destroy(&multi->sockhash);
   Curl_hash_destroy(&multi->hostcache);
   Curl_conncache_destroy(&multi->conn_cache);
   Curl_llist_destroy(&multi->msglist, NULL);
@@ -554,16 +574,16 @@ CURLMcode curl_multi_add_handle(struct Curl_multi *multi,
 #if 0
 /* Debug-function, used like this:
  *
- * Curl_hash_print(multi->sockhash, debug_print_sock_hash);
+ * Curl_hash_print(&multi->sockhash, debug_print_sock_hash);
  *
  * Enable the hash print function first by editing hash.c
  */
 static void debug_print_sock_hash(void *p)
 {
   struct Curl_sh_entry *sh = (struct Curl_sh_entry *)p;
 
-  fprintf(stderr, " [easy %p/magic %x/socket %d]",
-          (void *)sh->data, sh->data->magic, (int)sh->socket);
+  fprintf(stderr, " [readers %u][writers %u]",
+          sh->readers, sh->writers);
 }
 #endif
 
@@ -576,7 +596,8 @@ static CURLcode multi_done(struct Curl_easy *data,
   struct connectdata *conn = data->conn;
   unsigned int i;
 
-  DEBUGF(infof(data, "multi_done"));
+  DEBUGF(infof(data, "multi_done: status: %d prem: %d done: %d",
+               (int)status, (int)premature, data->state.done));
 
   if(data->state.done)
     /* Stop if multi_done() has already been called */
@@ -2694,7 +2715,7 @@ CURLMcode curl_multi_cleanup(struct Curl_multi *multi)
     /* Close all the connections in the connection cache */
     Curl_conncache_close_all_connections(&multi->conn_cache);
 
-    Curl_hash_destroy(&multi->sockhash);
+    sockhash_destroy(&multi->sockhash);
     Curl_conncache_destroy(&multi->conn_cache);
     Curl_llist_destroy(&multi->msglist, NULL);
     Curl_llist_destroy(&multi->pending, NULL);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-736b7e182e` → 本草稿移入 `cases/defect/auto-curl-736b7e182e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
