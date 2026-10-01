# auto-curl-5e422834b1

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
| 外部依赖数（dep_count） | 62 |
| 编译错误数（gcc syntax-only） | 32（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23007 (https://github.com/curl/curl/pull/23007)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 940；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 23007 mqtt: drain queued output before advancing the state machine :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -67,8 +67,10 @@ enum mqttstate {
   MQTT_SUBACK_COMING,     /* 4 - the SUBACK remainder */
   MQTT_PUBWAIT,    /* 5 - wait for publish */
   MQTT_PUB_REMAIN,  /* 6 - wait for the remainder of the publish */
+  MQTT_POST_DRAIN,        /* 7 - finish sending PUBLISH */
+  MQTT_DISCONNECT_DRAIN,  /* 8 - finish sending DISCONNECT */
 
-  MQTT_NOSTATE /* 7 - never used an actual state */
+  MQTT_NOSTATE /* 9 - never used an actual state */
 };
 
 struct mqtt_conn {
@@ -140,32 +142,48 @@ static CURLcode mqtt_send(struct Curl_easy *data,
   if(!mq)
     return CURLE_FAILED_INIT;
 
+  /* A new packet must not replace a previously queued packet tail. */
+  DEBUGASSERT(!curlx_dyn_len(&mq->sendbuf));
   result = Curl_xfer_send(data, buf, len, FALSE, &n);
   if(result)
     return result;
   mq->lastTime = *Curl_pgrs_now(data);
   Curl_debug(data, CURLINFO_HEADER_OUT, buf, n);
-  if(len != n) {
-    size_t nsend = len - n;
-    if(curlx_dyn_len(&mq->sendbuf)) {
-      DEBUGASSERT(curlx_dyn_len(&mq->sendbuf) >= nsend);
-      result = curlx_dyn_tail(&mq->sendbuf, nsend); /* keep this much */
-    }
-    else {
-      result = curlx_dyn_addn(&mq->sendbuf, &buf[n], nsend);
-    }
-  }
-  else
-    curlx_dyn_reset(&mq->sendbuf);
+  if(len != n)
+    result = curlx_dyn_addn(&mq->sendbuf, &buf[n], len - n);
   return result;
 }
 
+/* Send the queued tail, preserving any bytes that still cannot be sent. */
+static CURLcode mqtt_flush(struct Curl_easy *data)
+{
+  struct MQTT *mq = Curl_meta_get(data, CURL_META_MQTT_EASY);
+  size_t len, n;
+  CURLcode result;
+
+  if(!mq)
+    return CURLE_FAILED_INIT;
+  len = curlx_dyn_len(&mq->sendbuf);
+  if(!len)
+    return CURLE_OK;
+
+  result = Curl_xfer_send(data, curlx_dyn_ptr(&mq->sendbuf), len, FALSE, &n);
+  if(result)
+    return result;
+  mq->lastTime = *Curl_pgrs_now(data);
+  Curl_debug(data, CURLINFO_HEADER_OUT, curlx_dyn_ptr(&mq->sendbuf), n);
+  return curlx_dyn_tail(&mq->sendbuf, len - n);
+}
+
 /* Generic function called by the multi interface to figure out what socket(s)
    to wait for and for what actions during the DOING and PROTOCONNECT
    states */
 static CURLcode mqtt_pollset(struct Curl_easy *data,
                              struct easy_pollset *ps)
 {
+  struct MQTT *mq = Curl_meta_get(data, CURL_META_MQTT_EASY);
+  if(mq && curlx_dyn_len(&mq->sendbuf))
+    return Curl_pollset_add_out(data, ps, data->conn->sock[FIRSTSOCKET]);
   return Curl_pollset_add_in(data, ps, data->conn->sock[FIRSTSOCKET]);
 }
 
@@ -637,6 +655,8 @@ static const char * const statenames[] = {
   "MQTT_SUBACK_COMING",
   "MQTT_PUBWAIT",
   "MQTT_PUB_REMAIN",
+  "MQTT_POST_DRAIN",
+  "MQTT_DISCONNECT_DRAIN",
 
   "NOT A STATE"
 };
@@ -843,14 +863,16 @@ static CURLcode mqtt_doing(struct Curl_easy *data, bool *done)
 
   if(curlx_dyn_len(&mq->sendbuf)) {
     /* send the remainder of an outgoing packet */
-    result = mqtt_send(data, curlx_dyn_ptr(&mq->sendbuf),
-                       curlx_dyn_len(&mq->sendbuf));
-    if(result)
+    result = mqtt_flush(data);
+    /* CURLE_OK can still mean a short write. Wait for writable progress
+       before sending another packet or processing a reply. */
+    if(result || curlx_dyn_len(&mq->sendbuf))
       return result;
   }
 
   result = mqtt_ping(data);
-  if(result)
+  /* Packet processing may send more output, so finish PINGREQ first. */
+  if(result || curlx_dyn_len(&mq->sendbuf))
     return result;
 
   infof(data, "mqtt_doing: state [%d]", (int)mqtt->state);
@@ -935,20 +957,31 @@ static CURLcode mqtt_doing(struct Curl_easy *data, bool *done)
     if(result)
       break;
 
-    if(data->state.httpreq == HTTPREQ_POST) {
-      result = mqtt_publish(data);
-      if(!result) {
-        result = mqtt_disconnect(data);
-        *done = TRUE;
-      }
-      mqtt->nextstate = MQTT_FIRST;
-    }
-    else {
+    if(data->state.httpreq != HTTPREQ_POST) {
       result = mqtt_subscribe(data);
-      if(!result) {
+      if(!result)
         mqstate(data, MQTT
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`curlx_dyn_addn`
- 外部函数：`curlx_dyn_len`
- 外部函数：`curlx_dyn_ptr`
- 外部函数：`curlx_dyn_reset`
- 外部函数：`curlx_dyn_tail`
- 外部函数：`curlx_free`
- 外部函数：`curlx_malloc`
- 外部函数：`curlx_ptimediff_ms`
- 外部函数：`curlx_sotouz_fits`
- 外部函数：`failf`
- 外部函数：`infof`
- 外部函数：`memcpy`
- 外部函数：`mqstate`
- 外部函数：`mqtt_pollset`
- 外部函数：`s`
- 外部函数：`strlen`
- 外部函数：`topic`
- 大写宏：`BIT`
- 大写宏：`CURLE_BAD_FUNCTION_ARGUMENT`
- 大写宏：`CURLE_FAILED_INIT`
- 大写宏：`CURLE_OK`
- 大写宏：`CURLE_OUT_OF_MEMORY`
- 大写宏：`CURLE_TOO_LARGE`
- 大写宏：`CURLE_URL_MALFORMAT`
- 大写宏：`CURLINFO_HEADER_OUT`
- 大写宏：`DEBUGASSERT`
- 大写宏：`DEBUGBUILD`
- 大写宏：`DEBUGF`
- 大写宏：`DOING`
- 大写宏：`FALSE`
- 大写宏：`FIRST`
- 大写宏：`FIRSTSOCKET`
- 大写宏：`HTTPREQ_POST`
- 大写宏：`MQTT`
- 大写宏：`MQTT_CONNACK`
- 大写宏：`MQTT_FIRST`
- 大写宏：`MQTT_NOSTATE`
- 大写宏：`MQTT_PUBWAIT`
- 大写宏：`MQTT_PUB_REMAIN`
- 大写宏：`MQTT_REMAINING_LENGTH`
- 大写宏：`MQTT_SUBACK`
- 大写宏：`MQTT_SUBACK_COMING`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PINGREQ`
- 大写宏：`PROTOCONNECT`
- 大写宏：`REJECT_CTRL`
- 大写宏：`STATE`
- 大写宏：`SUBACK`
- 大写宏：`TRUE`
- 大写宏：`URL`
- 外部类型：`CURLcode`
- 外部类型：`Curl_easy`
- 外部类型：`Forgot`
- 外部类型：`Generic`
- 外部类型：`No`
- 外部类型：`QoS`
- 外部类型：`Too`
- 外部类型：`connectdata`
- 外部类型：`curl_off_t`
- 外部类型：`curltime`
- 外部类型：`dynbuf`
- 外部类型：`easy_pollset`
- 外部类型：`off_t`
- 外部类型：`size_t`
- 外部类型：`timediff_t`

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

1. 完成上面检查清单后评论 `/case accept auto-curl-5e422834b1` → 本草稿移入 `cases/defect/auto-curl-5e422834b1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
