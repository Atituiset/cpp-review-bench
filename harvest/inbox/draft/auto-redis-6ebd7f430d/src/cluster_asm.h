// AUTO-DRAFT from redis/redis PR #15302
int asmNotifyConfigUpdated(struct asmTask *task, sds *err);
size_t asmGetPeakSyncBufferSize(void);
size_t asmGetImportInputBufferSize(void);
size_t asmGetMigrateOutputBufferSize(void);  // <<< BUG ANCHOR
int clusterAsmCancel(const char *task_id, const char *reason);
int clusterAsmCancelBySlot(int slot, const char *reason);
int clusterAsmCancelBySlotRangeArray(struct slotRangeArray *slots, const char *reason);
