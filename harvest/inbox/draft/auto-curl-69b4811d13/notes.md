# auto-curl-69b4811d13

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | curl/curl |
| 源 PR | [#23007](https://github.com/curl/curl/pull/23007) |
| 许可证 | MIT |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23007 (https://github.com/curl/curl/pull/23007)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 427；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 23007 mqtt: drain queued output before advancing the state machine :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -413,6 +413,25 @@ static int publish(FILE *dump,
 
 static char topic[MAX_TOPIC_LENGTH + 1];
 
+static bool readall(curl_socket_t fd, unsigned char *buffer, size_t len)
+{
+  size_t nread = 0;
+
+  while(nread < len) {
+    ssize_t rc = sread(fd, buffer + nread, len - nread);
+    if(rc <= 0) {
+      if(rc < 0 && SOCKERRNO == SOCKEINTR && !got_exit_signal)
+        continue;
+      logmsg("READ %zd bytes [SHORT!]", rc);
+      return FALSE;
+    }
+    logmsg("READ %zd bytes", rc);
+    loghex(buffer + nread, rc);
+    nread += (size_t)rc;
+  }
+  return TRUE;
+}
+
 static bool fixedheader(curl_socket_t fd,
                         unsigned char *bytep,
                         size_t *remaining_lengthp,
@@ -421,23 +440,18 @@ static bool fixedheader(curl_socket_t fd,
   /* get the fixed header */
   unsigned char buffer[10];
 
-  /* get the first two bytes */
-  ssize_t rc = sread(fd, buffer, 2);
   size_t i;
-  if(rc < 2) {
-    logmsg("READ %zd bytes [SHORT!]", rc);
+
+  /* get the first two bytes */
+  if(!readall(fd, buffer, 2))
     return FALSE; /* fail */
-  }
-  logmsg("READ %zd bytes", rc);
-  loghex(buffer, rc);
   *bytep = buffer[0];
 
   /* if the length byte has the top bit set, get the next one too */
   i = 1;
   while(buffer[i] & 0x80) {
     i++;
-    rc = sread(fd, &buffer[i], 1);
-    if(rc != 1) {
+    if(!readall(fd, &buffer[i], 1)) {
       logmsg("Remaining Length broken");
       return FALSE;
     }
@@ -513,14 +527,10 @@ static curl_socket_t mqttit(curl_socket_t fd)
       buffer = newbuffer;
     }
 
-    if(remaining_length) {
-      /* reading variable header and payload into buffer */
-      rc = sread(fd, buffer, remaining_length);
-      if(rc > 0) {
-        logmsg("READ %zd bytes", rc);
-        loghex(buffer, rc);
-      }
-    }
+    /* Read the complete variable header and payload. */
+    if(!readall(fd, buffer, remaining_length))
+      goto end;
+    rc = (ssize_t)remaining_length;
 
     if(byte == MQTT_MSG_CONNECT) {
       logprotocol(FROM_CLIENT, "CONNECT", remaining_length, dump, buffer, rc);
@@ -658,11 +668,9 @@ static curl_socket_t mqttit(curl_socket_t fd)
 #endif
       /* expect a disconnect here */
       /* get the request */
-      rc = sread(fd, &buffer[0], 2);
-
-      logmsg("READ %zd bytes [DISCONNECT]", rc);
-      loghex(buffer, rc);
-      logprotocol(FROM_CLIENT, "DISCONNECT", 0, dump, buffer, rc);
+      if(!readall(fd, buffer, 2))
+        goto end;
+      logprotocol(FROM_CLIENT, "DISCONNECT", 0, dump, buffer, 2);
       goto end;
     }
     else {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`fprintf`
- 外部函数：`loghex`
- 外部函数：`logmsg`
- 外部函数：`snprintf`
- 外部函数：`sread`
- 大写宏：`CONNECT`
- 大写宏：`DISCONNECT`
- 大写宏：`FALSE`
- 大写宏：`FILE`
- 大写宏：`FROM_CLIENT`
- 大写宏：`FROM_SERVER`
- 大写宏：`READ`
- 大写宏：`SHORT`
- 大写宏：`TRUE`
- 外部类型：`Length`
- 外部类型：`Remaining`
- 外部类型：`curl_socket_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-69b4811d13` → 本草稿移入 `cases/defect/auto-curl-69b4811d13/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
