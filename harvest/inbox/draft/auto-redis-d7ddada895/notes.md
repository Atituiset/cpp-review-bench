# auto-redis-d7ddada895

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15203 (https://github.com/redis/redis/pull/15203)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 420；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -134,6 +134,9 @@ static uint64_t VectorSetTypeNextId = 0;
 // Default num elements returned by VSIM.
 #define VSET_DEFAULT_COUNT 10
 
+// Maximum allowed vector dimension for input vectors and sets.
+#define VSET_MAX_VECTOR_DIM (1<<16)
+
 /* ========================== Internal data structure  ====================== */
 
 /* Our abstract data type needs a dual representation similar to Redis
@@ -408,6 +411,7 @@ float *parseVector(RedisModuleString **argv, int argc, int start_idx,
         // Must be 4 bytes per component.
         if (vec_raw_len % 4 || vec_raw_len < 4) return NULL;
         *dim = vec_raw_len/4;
+        if (*dim > VSET_MAX_VECTOR_DIM) return NULL;
 
         vec = RedisModule_Alloc(vec_raw_len);
         if (!vec) return NULL;
@@ -417,7 +421,7 @@ float *parseVector(RedisModuleString **argv, int argc, int start_idx,
         if (argc < start_idx + 2) return NULL;  // Need at least the dimension.
         long long vdim; // Vector dimension passed by the user.
         if (RedisModule_StringToLongLong(argv[start_idx+1],&vdim)
-            != REDISMODULE_OK || vdim < 1) return NULL;
+            != REDISMODULE_OK || vdim < 1 || vdim > VSET_MAX_VECTOR_DIM) return NULL;
 
         // Check that all the arguments are available.
         if (argc < start_idx + 2 + vdim) return NULL;
@@ -441,6 +445,12 @@ float *parseVector(RedisModuleString **argv, int argc, int start_idx,
         return NULL;  // Unknown format.
     }
 
+    // reduce_dim must be <= dim
+    if (reduce_dim && *reduce_dim && *reduce_dim > *dim) {
+        if (vec) RedisModule_Free(vec);
+        return NULL;
+    }
+
     if (consumed_args) *consumed_args = consumed;
     return vec;
 }
@@ -1966,6 +1976,15 @@ void *VectorSetRdbLoad(RedisModuleIO *rdb, int encver) {
     uint32_t quant_type = hnsw_config & 0xff;
     uint32_t hnsw_m = (hnsw_config >> 8) & 0xffff;
 
+    /* Validate dimension loaded from RDB to enforce invariants and
+     * avoid absurd allocations or inconsistent state. */
+    if (dim == 0 || dim > VSET_MAX_VECTOR_DIM) {
+        RedisModule_LogIOError(rdb, "warning",
+            "Invalid vector dimension in RDB: dim=%u (max allowed %u)",
+            (unsigned)dim, (unsigned)VSET_MAX_VECTOR_DIM);
+        return NULL;
+    }
+
     /* Check that the quantization type is correct. Otherwise
      * return ASAP signaling the error. */
     if (quant_type != HNSW_QUANT_NONE &&
@@ -1987,14 +2006,44 @@ void *VectorSetRdbLoad(RedisModuleIO *rdb, int encver) {
         uint32_t input_dim = RedisModule_LoadUnsigned(rdb);
         if (RedisModule_IsIOError(rdb)) goto ioerr;
         uint32_t output_dim = dim;
-        size_t matrix_size = sizeof(float) * input_dim * output_dim;
 
-        vset->proj_matrix = RedisModule_Alloc(matrix_size);
-        vset->proj_input_size = input_dim;
+        /* Sanity check projection dimensions. */
+        if (input_dim == 0 || output_dim == 0 || input_dim > VSET_MAX_VECTOR_DIM || output_dim > input_dim) {
+            RedisModule_LogIOError(rdb, "warning",
+                "Invalid projection matrix dimensions: input_dim=%u, output_dim=%u (max allowed %u)",
+                (unsigned)input_dim, (unsigned)output_dim,
+                (unsigned)VSET_MAX_VECTOR_DIM);
+            goto ioerr;
+        }
+
+        /* Check for overflow in matrix_size = sizeof(float) * input_dim * output_dim. */
+        #if SIZE_MAX == UINT32_MAX
+            uint64_t product = (uint64_t) output_dim * (uint64_t) input_dim * sizeof(float);
+            if (product > SIZE_MAX) {
+                RedisModule_LogIOError(rdb, "warning",
+                    "Projection matrix size overflow (output_dim too large): input_dim=%u, output_dim=%u",
+                    (unsigned)input_dim, (unsigned)output_dim);
+                goto ioerr;
+            }
+        #endif
+
+        size_t matrix_size = sizeof(float) * (size_t)input_dim * (size_t)output_dim;
 
-        // Load projection matrix as a binary blob
-        char *matri
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-d7ddada895` → 本草稿移入 `cases/defect/auto-redis-d7ddada895/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
