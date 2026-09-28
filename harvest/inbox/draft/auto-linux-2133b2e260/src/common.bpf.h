// AUTO-DRAFT from torvalds/linux PR #7cdf91542e1ed03aa8f5ab029702b3abde5be6d5
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
void scx_bpf_events(struct scx_event_stats *events, size_t events__sz) __ksym __weak;
s32 scx_bpf_cpu_to_cid(s32 cpu) __ksym __weak;
s32 scx_bpf_cid_to_cpu(s32 cid) __ksym __weak;
void scx_bpf_cid_topo(s32 cid, struct scx_cid_topo *out) __ksym __weak;
void scx_bpf_kick_cid(s32 cid, u64 flags) __ksym __weak;
s32 scx_bpf_task_cid(const struct task_struct *p) __ksym __weak;
s32 scx_bpf_this_cid(void) __ksym __weak;
