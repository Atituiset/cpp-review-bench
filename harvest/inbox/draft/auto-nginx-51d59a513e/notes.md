# auto-nginx-51d59a513e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1756](https://github.com/nginx/nginx/pull/1756) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1756 (https://github.com/nginx/nginx/pull/1756)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 49；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1756 QUIC: support BPF reload and binary upgrade :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -44,15 +44,6 @@ char _license[] SEC("license") = LICENSE;
 #define NGX_QUIC_SERVER_CID_LEN  20
 
 
-#define advance_data(nbytes)                                                  \
-    offset += nbytes;                                                         \
-    if (start + offset > end) {                                               \
-        debugmsg("cannot read %ld bytes at offset %ld", nbytes, offset);      \
-        goto failed;                                                          \
-    }                                                                         \
-    data = start + offset - 1;
-
-
 #define ngx_quic_parse_uint64(p)                                              \
     (((__u64)(p)[0] << 56) |                                                  \
      ((__u64)(p)[1] << 48) |                                                  \
@@ -63,78 +54,154 @@ char _license[] SEC("license") = LICENSE;
      ((__u64)(p)[6] << 8)  |                                                  \
      ((__u64)(p)[7]))
 
+#define ngx_quic_bpf_listen_key(worker)                                       \
+    (((__u64) 0xFF << 56) | ((__u64) (worker) & 0xFFFFFFFFFFFFULL))
+
+
 /*
  * actual map object is created by the "bpf" system call,
  * all pointers to this variable are replaced by the bpf loader
  */
-extern int ngx_quic_sockmap;
+struct {} ngx_quic_sockmap SEC(".maps");
+struct {} ngx_quic_worker_counts SEC(".maps");
 
 
 SEC(PROGNAME)
 int ngx_quic_select_socket_by_dcid(struct sk_reuseport_md *ctx)
 {
-    int             rc;
+    int             rc, worker_idx;
+    __u32           wc_key, *wc0, *wc1, n, m;
     __u64           key;
-    size_t          len, offset;
-    unsigned char  *start, *end, *data, *dcid;
+    size_t          offset;
+    unsigned char  *start, *end, *dcid, byte, buf[NGX_QUIC_SERVER_CID_LEN];
 
-    start = ctx->data;
+    start = (unsigned char *) ctx->data;
     end = (unsigned char *) ctx->data_end;
-    offset = 0;
+    offset = sizeof(struct udphdr);
+
+    if (start + offset >= end) {
+
+        if (bpf_skb_load_bytes(ctx, offset, &byte, 1)) {
+            goto failed;
+        }
+
+    } else {
+        byte = start[offset];
+    }
 
-    advance_data(sizeof(struct udphdr)); /* data at UDP header */
-    advance_data(1); /* data at QUIC flags */
+    if (byte & NGX_QUIC_PKT_LONG) {
 
-    if (data[0] & NGX_QUIC_PKT_LONG) {
+        offset += 5;
 
-        advance_data(4); /* data at QUIC version */
-        advance_data(1); /* data at DCID len */
+        if (start + offset >= end) {
 
-        len = data[0];   /* read DCID length */
+            if (bpf_skb_load_bytes(ctx, offset, &byte, 1)) {
+                goto failed;
+            }
 
-        if (len < 8) {
-            /* it's useless to search for key in such short DCID */
-            return SK_PASS;
+        } else {
+            byte = start[offset];
         }
 
-    } else {
-        len = NGX_QUIC_SERVER_CID_LEN;
+        if (byte != NGX_QUIC_SERVER_CID_LEN) {
+            goto new;
+        }
     }
 
-    dcid = &data[1];
-    advance_data(len); /* we expect the packet to have full DCID */
+    offset++;
+
+    if (start + offset + NGX_QUIC_SERVER_CID_LEN > end) {
+
+        if (bpf_skb_load_bytes(ctx, offset, buf, NGX_QUIC_SERVER_CID_LEN)) {
+            goto failed;
+        }
+
+        dcid = buf;
 
-    /* make verifier happy */
-    if (dcid + sizeof(__u64) > end) {
-        goto failed;
+    } else {
+        dcid = start + offset;
     }
 
     key = ngx_quic_parse_uint64(dcid);
 
+    if ((key >> 56) == 0xFF) {
+        goto new;
+    }
+
     rc = bpf_sk_select_reuseport(ctx, &ngx_quic_sockmap, &key, 0);
 
-    switch (rc) {
-    case 0:
-        debugmsg("nginx quic socket selected by key 0x%llx", key);
+    if (rc == 0) {
+        debugmsg("nginx quic worker socket selected by dcid");
         return SK_PASS;
+    }
+
+    if (rc != -ENOENT) {
+        debugmsg("nginx quic bpf_sk_select_reuseport() failed: %d", r
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`advance_data`
- 外部函数：`bpf_sk_select_reuseport`
- 外部函数：`bpf_trace_printk`
- 外部函数：`debugmsg`
- 外部函数：`ngx_quic_parse_uint64`
- 大写宏：`DCID`
- 大写宏：`ENOENT`
- 大写宏：`ICMP`
- 大写宏：`PROGNAME`
- 大写宏：`QUIC`
- 大写宏：`SEC`
- 大写宏：`SK_DROP`
- 大写宏：`SK_PASS`
- 大写宏：`UDP`
- 外部类型：`size_t`
- 外部类型：`sk_reuseport_md`
- 外部类型：`udphdr`

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

1. 完成上面检查清单后评论 `/case accept auto-nginx-51d59a513e` → 本草稿移入 `cases/defect/auto-nginx-51d59a513e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
