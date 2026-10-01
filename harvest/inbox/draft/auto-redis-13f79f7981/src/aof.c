// AUTO-DRAFT from redis/redis PR #15191
return 1;
}

int rewriteGCRAObject(rio *r, robj *key, robj *o) {
    long long val;
    getLongLongFromGCRAObject(o, &val);
    if (rioWriteBulkLongLong(r,val) == 0) return 0;
    return 1;
}

/* Call the module type callback in order to rewrite a data type
 * that is exported by a module and is not handled by Redis itself.
        if (rewriteHashObject(r,key,o) == 0) return C_ERR;
    } else if (o->type == OBJ_STREAM) {
        if (rewriteStreamObject(r,key,o) == 0) return C_ERR;
    } else if (o->type == OBJ_GCRA) {
        if (rewriteGCRAObject(r,key,o) == 0) return C_ERR;
    } else if (o->type == OBJ_ARRAY) {
        if (rewriteArrayObject(r,key,o) == 0) return C_ERR;
    } else if (o->type == OBJ_MODULE) {
