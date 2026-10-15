// AUTO-DRAFT from redis/redis PR #15436
RedisModuleIO io;
    moduleValue *mv = o->ptr;
    moduleType *mt = mv->type;
    moduleInitIOContext(&io, &mt->entity, r, key, dbid);
    mt->aof_rewrite(&io,key,mv->value);
    if (io.ctx) {
