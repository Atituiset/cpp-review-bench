// AUTO-DRAFT from redis/redis PR #14831
zfree(io.ctx);
    }
    if (io.error) {
        return NULL;
    } else {
        robj *str = createObject(OBJ_STRING,payload.io.buffer.ptr);
