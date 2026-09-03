// AUTO-DRAFT from redis/redis PR #15005
}
  // <<< BUG ANCHOR
static int updateMemoryTrackingEnabled(const char **err) {
    int memory_tracking_enabled = server.key_memory_histograms || clusterSlotStatsEnabled(CLUSTER_SLOT_STATS_MEM);
    if (!server.memory_tracking_enabled && memory_tracking_enabled) {
        *err = "memory tracking cannot be enabled at runtime";
        return 0;
