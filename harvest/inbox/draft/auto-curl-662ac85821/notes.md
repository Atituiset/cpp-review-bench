# auto-curl-662ac85821

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #7552 (https://github.com/curl/curl/pull/7552)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 704；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -80,13 +80,33 @@
 #define HAVE_CARES_CALLBACK_TIMEOUTS 1
 #endif
 
+#if ARES_VERSION >= 0x010601
+/* IPv6 supported since 1.6.1 */
+#define HAVE_CARES_IPV6 1
+#endif
+
+#if ARES_VERSION >= 0x010704
+#define HAVE_CARES_SERVERS_CSV 1
+#define HAVE_CARES_LOCAL_DEV 1
+#define HAVE_CARES_SET_LOCAL 1
+#endif
+
+#if ARES_VERSION >= 0x010b00
+#define HAVE_CARES_PORTS_CSV 1
+#endif
+
+#if ARES_VERSION >= 0x011000
+/* 1.16.0 or later has ares_getaddrinfo */
+#define HAVE_CARES_GETADDRINFO 1
+#endif
+
 /* The last 3 #include files should be in this order */
 #include "curl_printf.h"
 #include "curl_memory.h"
 #include "memdebug.h"
 
 struct thread_data {
-  int num_pending; /* number of ares_gethostbyname() requests */
+  int num_pending; /* number of outstanding c-ares requests */
   struct Curl_addrinfo *temp_ai; /* intermediary result while fetching c-ares
                                     parts */
   int last_status;
@@ -490,6 +510,8 @@ CURLcode Curl_resolver_wait_resolv(struct Curl_easy *data,
   return result;
 }
 
+#ifndef HAVE_CARES_GETADDRINFO
+
 /* Connects results to the list */
 static void compound_results(struct thread_data *res,
                              struct Curl_addrinfo *ai)
@@ -620,7 +642,97 @@ static void query_completed_cb(void *arg,  /* (struct connectdata *) */
     }
   }
 }
+#else
+/* c-ares 1.16.0 or later */
 
+/*
+ * ares2addr() converts an address list provided by c-ares to an internal
+ * libcurl compatible list
+ */
+static struct Curl_addrinfo *ares2addr(struct ares_addrinfo_node *node)
+{
+  /* traverse the ares_addrinfo_node list */
+  struct ares_addrinfo_node *ai;
+  struct Curl_addrinfo *cafirst = NULL;
+  struct Curl_addrinfo *calast = NULL;
+  int error = 0;
+
+  for(ai = node; ai != NULL; ai = ai->ai_next) {
+    size_t ss_size;
+    struct Curl_addrinfo *ca;
+    /* ignore elements with unsupported address family, */
+    /* settle family-specific sockaddr structure size.  */
+    if(ai->ai_family == AF_INET)
+      ss_size = sizeof(struct sockaddr_in);
+#ifdef ENABLE_IPV6
+    else if(ai->ai_family == AF_INET6)
+      ss_size = sizeof(struct sockaddr_in6);
+#endif
+    else
+      continue;
+
+    /* ignore elements without required address info */
+    if(!ai->ai_addr || !(ai->ai_addrlen > 0))
+      continue;
+
+    /* ignore elements with bogus address size */
+    if((size_t)ai->ai_addrlen < ss_size)
+      continue;
+
+    ca = malloc(sizeof(struct Curl_addrinfo) + ss_size);
+    if(!ca) {
+      error = EAI_MEMORY;
+      break;
+    }
+
+    /* copy each structure member individually, member ordering, */
+    /* size, or padding might be different for each platform.    */
+
+    ca->ai_flags     = ai->ai_flags;
+    ca->ai_family    = ai->ai_family;
+    ca->ai_socktype  = ai->ai_socktype;
+    ca->ai_protocol  = ai->ai_protocol;
+    ca->ai_addrlen   = (curl_socklen_t)ss_size;
+    ca->ai_addr      = NULL;
+    ca->ai_canonname = NULL;
+    ca->ai_next      = NULL;
+
+    ca->ai_addr = (void *)((char *)ca + sizeof(struct Curl_addrinfo));
+    memcpy(ca->ai_addr, ai->ai_addr, ss_size);
+
+    /* if the return list is empty, this becomes the first element */
+    if(!cafirst)
+      cafirst = ca;
+
+    /* add this element last in the return list */
+    if(calast)
+      calast->ai_next = ca;
+    calast = ca;
+  }
+
+  /* if we failed, destroy the Curl_addrinfo list */
+  if(error) {
+    Curl_freeaddrinfo(cafirst);
+    cafirst = NULL;
+  }
+
+  return cafirst;
+}
+
+static void addrinfo_cb(void *arg, int status, int timeouts,
+                        struct ares_addrinfo *result)
+{
+  struct Curl_easy *data = (struct Curl_easy *)arg;
+  struct thread_data *res = data->state.async.tdata;
+  (void)timeouts;
+  if(ARES_SUCCESS == status) {
+    res->temp_ai = ares2addr(result->nodes);
+    res->last_status = CURL_ASYNC_SUCCESS;
+  }
+  res->num_pending--;
+}
+
+#endif
 /*
  * Curl_resolver_getaddrinfo() - when using ares
  *
@@ -658,8 +770,28 @@ struct Curl_addrinfo *Curl_reso
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-662ac85821` → 本草稿移入 `cases/defect/auto-curl-662ac85821/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
