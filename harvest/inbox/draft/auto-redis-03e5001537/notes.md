# auto-redis-03e5001537

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15917](https://github.com/redis/redis/pull/15917) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 80 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15917 (https://github.com/redis/redis/pull/15917)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15917 Fix Linux TCP connect error queue in hiredis :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -13,6 +13,16 @@
 #include <limits.h>
 #include <math.h>
 
+#ifdef __linux__
+#include <arpa/inet.h>
+#include <linux/errqueue.h>
+#include <linux/net_tstamp.h>
+#include <netinet/in.h>
+#include <sys/epoll.h>
+#include <sys/socket.h>
+#include <unistd.h>
+#endif
+
 #include "hiredis.h"
 #include "async.h"
 #include "adapters/poll.h"
@@ -946,6 +956,158 @@ static void test_blocking_connection_errors(void) {
 #endif
 }
 
+#ifdef __linux__
+static uint32_t wait_for_epollerr(int epoll_fd) {
+    struct epoll_event event;
+    int count;
+
+    do {
+        count = epoll_wait(epoll_fd, &event, 1, 2000);
+    } while (count == -1 && errno == EINTR);
+
+    assert(count == 1);
+    assert(event.events & EPOLLERR);
+    return event.events;
+}
+
+static void consume_timestamp_error(redisFD fd) {
+    union {
+        struct cmsghdr align;
+        unsigned char data[512];
+    } control;
+    struct sock_extended_err *socket_error = NULL;
+    struct msghdr message = {0};
+    struct iovec iov;
+    struct cmsghdr *cmsg;
+    char byte;
+
+    iov.iov_base = &byte;
+    iov.iov_len = sizeof(byte);
+    message.msg_iov = &iov;
+    message.msg_iovlen = 1;
+    message.msg_control = control.data;
+    message.msg_controllen = sizeof(control.data);
+
+    assert(recvmsg(fd, &message, MSG_ERRQUEUE | MSG_DONTWAIT) >= 0);
+    assert(!(message.msg_flags & MSG_CTRUNC));
+    for (cmsg = CMSG_FIRSTHDR(&message); cmsg != NULL;
+         cmsg = CMSG_NXTHDR(&message, cmsg)) {
+        if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
+            socket_error = (struct sock_extended_err *)CMSG_DATA(cmsg);
+            break;
+        }
+    }
+    assert(socket_error != NULL);
+    assert(socket_error->ee_origin == SO_EE_ORIGIN_TIMESTAMPING);
+}
+
+static void test_nonblocking_connect_error_queue(void) {
+    const int timestamping = SOF_TIMESTAMPING_TX_SOFTWARE |
+                             SOF_TIMESTAMPING_SOFTWARE |
+                             SOF_TIMESTAMPING_OPT_TSONLY;
+    const char pong[] = "+PONG\r\n";
+    struct sockaddr_in address = {0};
+    struct epoll_event event = {0};
+    redisContext *context;
+    redisReply *reply = NULL;
+    socklen_t address_length = sizeof(address);
+    socklen_t option_length;
+    char byte = 'x';
+    int listener_fd;
+    int peer_fd;
+    int epoll_fd;
+    int completed = 0;
+    int option;
+    int count;
+    ssize_t result;
+
+    listener_fd = socket(AF_INET, SOCK_STREAM, 0);
+    assert(listener_fd != -1);
+    address.sin_family = AF_INET;
+    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
+    address.sin_port = 0;
+    assert(bind(listener_fd, (struct sockaddr *)&address, sizeof(address)) == 0);
+    assert(listen(listener_fd, 1) == 0);
+    assert(getsockname(listener_fd, (struct sockaddr *)&address,
+                       &address_length) == 0);
+
+    context = redisConnectNonBlock("127.0.0.1", ntohs(address.sin_port));
+    assert(context != NULL && context->err == 0);
+    peer_fd = accept(listener_fd, NULL, NULL);
+    assert(peer_fd != -1);
+
+    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
+    assert(epoll_fd != -1);
+    event.events = EPOLLIN;
+    event.data.fd = context->fd;
+    assert(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, context->fd, &event) == 0);
+    count = setsockopt(context->fd, SOL_SOCKET, SO_TIMESTAMPING,
+                       &timestamping, sizeof(timestamping));
+    if (count == -1 && errno == ENOPROTOOPT) {
+        test("Nonblocking connect error queue (SO_TIMESTAMPING unavailable): ");
+        test_skipped();
+        goto cleanup;
+    }
+    assert(count == 0);
+
+    /* Timestamp records use the same sk_error_queue occupancy predicate as
+     * queued ICMP errors, without requiring raw sockets or a network race. */
+    assert(send(context->fd, &byte, sizeof(byte), 0) == (ssize_t)sizeof(byte));
+    assert(recv(peer_fd, &byte, sizeof(byte), 0) == (ssize_t)sizeof(byte));
+    (void)wait_for_epollerr(epoll_fd);
+    consume_times
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`freeReplyObject`
- 外部函数：`memcmp`
- 外部函数：`printf`
- 外部函数：`redisReaderCreate`
- 外部函数：`redisReaderFeed`
- 外部函数：`redisReaderFree`
- 外部函数：`redisReaderGetReply`
- 外部函数：`snprintf`
- 外部函数：`strcasecmp`
- 外部函数：`strlen`
- 外部函数：`test_cond`
- 大写宏：`LLONG_MAX`
- 大写宏：`LLONG_MIN`
- 大写宏：`LOLWUT`
- 大写宏：`NULL`
- 大写宏：`REDIS_ERR`
- 大写宏：`REDIS_OK`
- 大写宏：`REDIS_REPLY_ARRAY`
- 大写宏：`REDIS_REPLY_INTEGER`
- 大写宏：`REDIS_REPLY_STRING`
- 大写宏：`SIZE_MAX`
- 外部类型：`Bad`
- 外部类型：`Bulk`
- 外部类型：`Can`
- 外部类型：`Correctly`
- 外部类型：`Don`
- 外部类型：`Error`
- 外部类型：`Keep`
- 外部类型：`Memory`
- 外部类型：`Multi`
- 外部类型：`Out`
- 外部类型：`Protocol`
- 外部类型：`Set`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-03e5001537` → 本草稿移入 `cases/defect/auto-redis-03e5001537/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
