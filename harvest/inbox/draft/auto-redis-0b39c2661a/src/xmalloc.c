// AUTO-DRAFT from redis/redis PR #15225
}

static void
#if defined(__GNUC__) && __GNUC__ >= 10
__attribute__((access(none, 2)))
#endif
hash_table_del(hashTable *tbl, void *ptr)
  new_ptr = realloc(ptr, new_size);
  if (new_ptr != NULL && new_ptr != ptr)
    {
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
      hash_table_del(xmalloc_table, ptr);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
      hash_table_add(xmalloc_table, new_ptr, (int)new_size, file, line, func);
