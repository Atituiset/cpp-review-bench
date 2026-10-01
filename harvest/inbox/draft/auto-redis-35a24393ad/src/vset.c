// AUTO-DRAFT from redis/redis PR #15170
// Default num elements returned by VSIM.
#define VSET_DEFAULT_COUNT 10
  // <<< BUG ANCHOR
/* ========================== Internal data structure  ====================== */

/* Our abstract data type needs a dual representation similar to Redis
        // Must be 4 bytes per component.
        if (vec_raw_len % 4 || vec_raw_len < 4) return NULL;
        *dim = vec_raw_len/4;

        vec = RedisModule_Alloc(vec_raw_len);
        if (!vec) return NULL;
        if (argc < start_idx + 2) return NULL;  // Need at least the dimension.
        long long vdim; // Vector dimension passed by the user.
        if (RedisModule_StringToLongLong(argv[start_idx+1],&vdim)
            != REDISMODULE_OK || vdim < 1) return NULL;

        // Check that all the arguments are available.
        if (argc < start_idx + 2 + vdim) return NULL;
        return NULL;  // Unknown format.
    }

    if (consumed_args) *consumed_args = consumed;
    return vec;
}
    uint32_t quant_type = hnsw_config & 0xff;
    uint32_t hnsw_m = (hnsw_config >> 8) & 0xffff;

    /* Check that the quantization type is correct. Otherwise
     * return ASAP signaling the error. */
    if (quant_type != HNSW_QUANT_NONE &&
        uint32_t input_dim = RedisModule_LoadUnsigned(rdb);
        if (RedisModule_IsIOError(rdb)) goto ioerr;
        uint32_t output_dim = dim;
        size_t matrix_size = sizeof(float) * input_dim * output_dim;

        vset->proj_matrix = RedisModule_Alloc(matrix_size);
        vset->proj_input_size = input_dim;

        // Load projection matrix as a binary blob
        char *matrix_blob = RedisModule_LoadStringBuffer(rdb, NULL);
        if (matrix_blob == NULL) goto ioerr;
        memcpy(vset->proj_matrix, matrix_blob, matrix_size);
        RedisModule_Free(matrix_blob);
    }
