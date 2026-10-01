# auto-nginx-0102cfe9ab

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
| 外部依赖数（dep_count） | 69 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1756 (https://github.com/nginx/nginx/pull/1756)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 277；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1756 QUIC: support BPF reload and binary upgrade :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6,6 +6,8 @@
 
 #include <ngx_config.h>
 #include <ngx_core.h>
+#include <ngx_event.h>
+#include <ngx_event_quic_connection.h>
 
 
 #define NGX_QUIC_BPF_VARNAME  "NGINX_BPF_MAPS"
@@ -23,42 +25,67 @@
 #define ngx_core_get_conf(cycle)                                              \
     (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module)
 
+#define ngx_quic_bpf_listen_key(worker)                                       \
+    (((uint64_t) 0xFF << 56)                                                  \
+     | ((uint64_t) (worker) & 0xFFFFFFFFFFFFULL))
+
 
 typedef struct {
     ngx_queue_t           queue;
-    int                   map_fd;
+
+    int                   sock_map;
+    int                   worker_counts_map;
 
     struct sockaddr      *sockaddr;
     socklen_t             socklen;
-    ngx_uint_t            unused;     /* unsigned  unused:1; */
+
+    ngx_array_t           listening;
 } ngx_quic_sock_group_t;
 
 
+typedef struct {
+    ngx_socket_t          fd;
+    uint64_t              key;
+    ngx_listening_t      *listening;
+    ngx_connection_t     *connection;
+} ngx_quic_bpf_listening_t;
+
+
 typedef struct {
     ngx_flag_t            enabled;
     ngx_uint_t            map_size;
+    ngx_uint_t            master_index;
+    u_char               *env;
     ngx_queue_t           groups;     /* of ngx_quic_sock_group_t */
 } ngx_quic_bpf_conf_t;
 
 
 static void *ngx_quic_bpf_create_conf(ngx_cycle_t *cycle);
+static char *ngx_quic_bpf_init_conf(ngx_cycle_t *cycle, void *conf);
 static ngx_int_t ngx_quic_bpf_module_init(ngx_cycle_t *cycle);
+static void ngx_quic_bpf_exit_master(ngx_cycle_t *cycle);
 
 static void ngx_quic_bpf_cleanup(void *data);
 static ngx_inline void ngx_quic_bpf_close(ngx_log_t *log, int fd,
     const char *name);
+static ngx_inline ngx_int_t ngx_quic_bpf_map_update(ngx_log_t *log, int fd,
+    const void *key, const void *value, const char *name);
 
 static ngx_quic_sock_group_t *ngx_quic_bpf_find_group(ngx_quic_bpf_conf_t *bcf,
     ngx_listening_t *ls);
 static ngx_quic_sock_group_t *ngx_quic_bpf_alloc_group(ngx_cycle_t *cycle,
-    struct sockaddr *sa, socklen_t socklen);
+    ngx_listening_t *ls);
 static ngx_quic_sock_group_t *ngx_quic_bpf_create_group(ngx_cycle_t *cycle,
     ngx_listening_t *ls);
+static ngx_int_t ngx_quic_bpf_inherit_fd(ngx_cycle_t *cycle, int fd);
 static ngx_quic_sock_group_t *ngx_quic_bpf_get_group(ngx_cycle_t *cycle,
     ngx_listening_t *ls);
 static ngx_int_t ngx_quic_bpf_group_add_socket(ngx_cycle_t *cycle,
     ngx_listening_t *ls);
-static uint64_t ngx_quic_bpf_socket_key(ngx_fd_t fd, ngx_log_t *log);
+static ngx_int_t ngx_quic_bpf_add_worker_socket(ngx_cycle_t *cycle,
+    ngx_quic_sock_group_t *grp, ngx_listening_t *ls);
+static ngx_int_t ngx_quic_bpf_publish_workers(ngx_cycle_t *cycle,
+    ngx_quic_sock_group_t *grp, ngx_uint_t worker_count);
 
 static ngx_int_t ngx_quic_bpf_export_maps(ngx_cycle_t *cycle);
 static ngx_int_t ngx_quic_bpf_import_maps(ngx_cycle_t *cycle);
@@ -82,7 +109,7 @@ static ngx_command_t  ngx_quic_bpf_commands[] = {
 static ngx_core_module_t  ngx_quic_bpf_module_ctx = {
     ngx_string("quic_bpf"),
     ngx_quic_bpf_create_conf,
-    NULL
+    ngx_quic_bpf_init_conf
 };
 
 
@@ -97,14 +124,16 @@ ngx_module_t  ngx_quic_bpf_module = {
     NULL,                                  /* init thread */
     NULL,                                  /* exit thread */
     NULL,                                  /* exit process */
-    NULL,                                  /* exit master */
+    ngx_quic_bpf_exit_master,              /* exit master */
     NGX_MODULE_V1_PADDING
 };
 
 
 static void *
 ngx_quic_bpf_create_conf(ngx_cycle_t *cycle)
 {
+    size_t                len;
+    u_char               *env;
     ngx_quic_bpf_conf_t  *bcf;
 
     bcf = ngx_pcalloc(cycle->pool, sizeof(ngx_quic_bpf_conf_t));
@@ -115,20 +144,66 @@ ngx_quic_bpf_create_conf(ngx_cycle_t *cycle)
     bcf->enabled = NGX_CONF_UNSET;
     bcf->map_
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`close`
- 外部函数：`fcntl`
- 外部函数：`ngx_bpf_load_program`
- 外部函数：`ngx_bpf_map_create`
- 外部函数：`ngx_bpf_program_link`
- 外部函数：`ngx_conf_init_value`
- 外部函数：`ngx_core_get_conf`
- 外部函数：`ngx_get_conf`
- 外部函数：`ngx_log_error`
- 外部函数：`ngx_memcpy`
- 外部函数：`ngx_palloc`
- 外部函数：`ngx_pcalloc`
- 外部函数：`ngx_pool_cleanup_add`
- 外部函数：`ngx_queue_data`
- 外部函数：`ngx_queue_head`
- 外部函数：`ngx_queue_init`
- 外部函数：`ngx_queue_insert_tail`
- 外部函数：`ngx_queue_sentinel`
- 外部函数：`ngx_quic_bpf_export_maps`
- 外部函数：`ngx_quic_bpf_get_conf`
- 外部函数：`ngx_quic_bpf_get_group`
- 外部函数：`ngx_quic_bpf_group_add_socket`
- 外部函数：`ngx_quic_bpf_import_maps`
- 外部函数：`ngx_quic_bpf_socket_key`
- 外部函数：`ngx_string`
- 大写宏：`BPF_MAP_TYPE_SOCKHASH`
- 大写宏：`FD_CLOEXEC`
- 大写宏：`F_GETFD`
- 大写宏：`F_SETFD`
- 大写宏：`NGINX_BPF_MAPS`
- 大写宏：`NGX_CONF_UNSET`
- 大写宏：`NGX_CONF_UNSET_UINT`
- 大写宏：`NGX_LOG_EMERG`
- 大写宏：`NGX_MODULE_V1_PADDING`
- 大写宏：`NGX_OK`
- 大写宏：`NULL`
- 外部类型：`ngx_core_conf_t`
- 外部类型：`ngx_core_module_t`
- 外部类型：`ngx_cycle_t`
- 外部类型：`ngx_fd_t`
- 外部类型：`ngx_flag_t`
- 外部类型：`ngx_int_t`
- 外部类型：`ngx_listening_t`
- 外部类型：`ngx_log_t`
- 外部类型：`ngx_pool_cleanup_t`
- 外部类型：`ngx_queue_t`
- 外部类型：`ngx_quic_bpf_conf_t`
- 外部类型：`ngx_quic_sock_group_t`
- 外部类型：`ngx_uint_t`
- 外部类型：`sockaddr`
- 外部类型：`socklen_t`
- 外部类型：`uint64_t`

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

1. 完成上面检查清单后评论 `/case accept auto-nginx-0102cfe9ab` → 本草稿移入 `cases/defect/auto-nginx-0102cfe9ab/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
