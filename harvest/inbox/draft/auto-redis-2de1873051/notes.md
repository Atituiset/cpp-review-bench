# auto-redis-2de1873051

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15917](https://github.com/redis/redis/pull/15917) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15917 (https://github.com/redis/redis/pull/15917)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 297；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15917 Fix Linux TCP connect error queue in hiredis :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -41,6 +41,7 @@
 #include <stdio.h>
 #include <limits.h>
 #include <stdlib.h>
+#include <time.h>
 
 #include "net.h"
 #include "sds.h"
@@ -52,6 +53,26 @@ void __redisSetError(redisContext *c, int type, const char *str);
 
 int redisContextUpdateCommandTimeout(redisContext *c, const struct timeval *timeout);
 
+#ifdef __linux__
+static void redisDrainSocketErrorQueue(redisFD fd) {
+    int saved_errno = errno;
+
+    for (;;) {
+        char buf[1];
+        struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
+        struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
+
+        if (recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) >= 0)
+            continue;
+        if (errno == EINTR)
+            continue;
+        break;
+    }
+
+    errno = saved_errno;
+}
+#endif
+
 void redisNetClose(redisContext *c) {
     if (c && c->fd != REDIS_INVALID_FD) {
         close(c->fd);
@@ -293,45 +314,77 @@ static int redisContextTimeoutMsec(redisContext *c, long *result)
     return REDIS_OK;
 }
 
+static long long redisPollMillis(void) {
+#ifndef _MSC_VER
+    struct timespec now;
+    clock_gettime(CLOCK_MONOTONIC, &now);
+    return ((long long)now.tv_sec * 1000) + now.tv_nsec / 1000000;
+#else
+    FILETIME ft;
+    GetSystemTimeAsFileTime(&ft);
+    return (((long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime) / 10000;
+#endif
+}
+
 static int redisContextWaitReady(redisContext *c, long msec) {
-    struct pollfd   wfd[1];
+    struct pollfd wfd;
+    long long end;
+    int res;
 
-    wfd[0].fd     = c->fd;
-    wfd[0].events = POLLOUT;
+    if (errno != EINPROGRESS) {
+        __redisSetErrorFromErrno(c,REDIS_ERR_IO,NULL);
+        redisNetClose(c);
+        return REDIS_ERR;
+    }
+
+    wfd.fd = c->fd;
+    wfd.events = POLLOUT;
+    end = msec >= 0 ? redisPollMillis() + msec : 0;
 
-    if (errno == EINPROGRESS) {
-        int res;
+    for (;;) {
+        int completed = 0;
 
-        if ((res = poll(wfd, 1, msec)) == -1) {
+        res = poll(&wfd, 1, msec);
+        if (res < 0 && errno != EINTR) {
             __redisSetErrorFromErrno(c, REDIS_ERR_IO, "poll(2)");
             redisNetClose(c);
             return REDIS_ERR;
-        } else if (res == 0) {
+        }
+
+        if (res > 0) {
+            if (redisCheckConnectDone(c, &completed) != REDIS_OK) {
+                redisCheckSocketError(c);
+                return REDIS_ERR;
+            }
+            if (completed)
+                return REDIS_OK;
+        }
+
+        if (res == 0) {
             errno = ETIMEDOUT;
-            __redisSetErrorFromErrno(c,REDIS_ERR_IO,NULL);
+            __redisSetErrorFromErrno(c, REDIS_ERR_IO, NULL);
             redisNetClose(c);
             return REDIS_ERR;
         }
 
-        if (redisCheckConnectDone(c, &res) != REDIS_OK || res == 0) {
-            redisCheckSocketError(c);
-            return REDIS_ERR;
-        }
+        if (msec >= 0) {
+            long long now = redisPollMillis();
 
-        return REDIS_OK;
+            if (now >= end) {
+                errno = ETIMEDOUT;
+                __redisSetErrorFromErrno(c, REDIS_ERR_IO, NULL);
+                redisNetClose(c);
+                return REDIS_ERR;
+            }
+            msec = (long)(end - now);
+        }
     }
-
-    __redisSetErrorFromErrno(c,REDIS_ERR_IO,NULL);
-    redisNetClose(c);
-    return REDIS_ERR;
 }
 
 int redisCheckConnectDone(redisContext *c, int *completed) {
     int rc = connect(c->fd, (const struct sockaddr *)c->saddr, c->addrlen);
-    if (rc == 0) {
-        *completed = 1;
-        return REDIS_OK;
-    }
+    if (rc == 0)
+        goto connected;
     int error = errno;
     if (error == EINPROGRESS) {
         /* must check error to see if connect failed.  Get the socket error */
@@ -341,8 +394,7 @@ int redisCheckConnectDone(redisContext *c, int *completed) {
         if (fail == 0) {
             if (so_error == 0) {
                 /* Socket is connected! */
-                *completed = 1;
-    
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__redisSetError`
- 外部函数：`close`
- 外部函数：`connect`
- 外部函数：`hi_malloc`
- 外部函数：`memcpy`
- 外部函数：`poll`
- 外部函数：`snprintf`
- 外部函数：`strerror_r`
- 大写宏：`EALREADY`
- 大写宏：`EINPROGRESS`
- 大写宏：`EISCONN`
- 大写宏：`ETIMEDOUT`
- 大写宏：`EWOULDBLOCK`
- 大写宏：`NULL`
- 大写宏：`POLLOUT`
- 大写宏：`REDIS_ERR`
- 大写宏：`REDIS_ERR_IO`
- 大写宏：`REDIS_INVALID_FD`
- 大写宏：`REDIS_OK`
- 外部类型：`Allocate`
- 外部类型：`Get`
- 外部类型：`Same`
- 外部类型：`Socket`
- 外部类型：`pollfd`
- 外部类型：`size_t`
- 外部类型：`sockaddr`
- 外部类型：`timeval`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-2de1873051` → 本草稿移入 `cases/defect/auto-redis-2de1873051/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
