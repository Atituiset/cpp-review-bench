// AUTO-DRAFT from redis/redis PR #15302
return 0;
}
  // <<< BUG ANCHOR
size_t asmGetMigrateOutputBufferSize(void) {
    if (!asmManager || listLength(asmManager->tasks) == 0) return 0;

    asmTask *task = listNodeValue(listFirst(asmManager->tasks));
