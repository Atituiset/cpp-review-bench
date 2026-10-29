# auto-nginx-55c8ad1434

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | nginx/nginx |
| 源 PR | [#1869](https://github.com/nginx/nginx/pull/1869) |
| 许可证 | BSD-2-Clause |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1869 (https://github.com/nginx/nginx/pull/1869)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 1869 Limit conn: fixed counter leak on memory allocation errors :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -59,6 +59,8 @@ static ngx_rbtree_node_t *ngx_stream_limit_conn_lookup(ngx_rbtree_t *rbtree,
     ngx_str_t *key, uint32_t hash);
 static void ngx_stream_limit_conn_cleanup(void *data);
 static ngx_inline void ngx_stream_limit_conn_cleanup_all(ngx_pool_t *pool);
+static void ngx_stream_limit_conn_cleanup_node(ngx_shm_zone_t *shm_zone,
+    ngx_rbtree_node_t *node);
 
 static ngx_int_t ngx_stream_limit_conn_status_variable(ngx_stream_session_t *s,
     ngx_stream_variable_value_t *v, uintptr_t data);
@@ -274,6 +276,7 @@ ngx_stream_limit_conn_handler(ngx_stream_session_t *s)
         cln = ngx_pool_cleanup_add(s->connection->pool,
                                    sizeof(ngx_stream_limit_conn_cleanup_t));
         if (cln == NULL) {
+            ngx_stream_limit_conn_cleanup_node(limits[i].shm_zone, node);
             return NGX_ERROR;
         }
 
@@ -414,6 +417,19 @@ ngx_stream_limit_conn_cleanup_all(ngx_pool_t *pool)
 }
 
 
+static void
+ngx_stream_limit_conn_cleanup_node(ngx_shm_zone_t *shm_zone,
+    ngx_rbtree_node_t *node)
+{
+    ngx_stream_limit_conn_cleanup_t  lccln;
+
+    lccln.shm_zone = shm_zone;
+    lccln.node = node;
+
+    ngx_stream_limit_conn_cleanup(&lccln);
+}
+
+
 static ngx_int_t
 ngx_stream_limit_conn_init_zone(ngx_shm_zone_t *shm_zone, void *data)
 {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ngx_log_debug2`
- 外部函数：`ngx_pool_cleanup_add`
- 外部函数：`ngx_rbtree_delete`
- 外部函数：`ngx_shmtx_lock`
- 外部函数：`ngx_shmtx_unlock`
- 外部函数：`ngx_slab_free_locked`
- 大写宏：`NGX_ERROR`
- 大写宏：`NGX_LOG_DEBUG_STREAM`
- 大写宏：`NGX_OK`
- 大写宏：`NULL`
- 外部类型：`ngx_int_t`
- 外部类型：`ngx_pool_cleanup_t`
- 外部类型：`ngx_pool_t`
- 外部类型：`ngx_rbtree_node_t`
- 外部类型：`ngx_rbtree_t`
- 外部类型：`ngx_shm_zone_t`
- 外部类型：`ngx_slab_pool_t`
- 外部类型：`ngx_str_t`
- 外部类型：`ngx_stream_complex_value_t`
- 外部类型：`ngx_stream_limit_conn_cleanup_t`
- 外部类型：`ngx_stream_limit_conn_ctx_t`
- 外部类型：`ngx_stream_limit_conn_node_t`
- 外部类型：`ngx_stream_limit_conn_shctx_t`
- 外部类型：`ngx_stream_session_t`
- 外部类型：`ngx_stream_variable_value_t`
- 外部类型：`uint32_t`
- 外部类型：`uintptr_t`

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

1. 完成上面检查清单后评论 `/case accept auto-nginx-55c8ad1434` → 本草稿移入 `cases/defect/auto-nginx-55c8ad1434/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
