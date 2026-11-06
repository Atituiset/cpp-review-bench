// AUTO-DRAFT from postgres/postgres PR #161d5ffcc5c2f446df528bd273de822b86965d92
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

extern bool check_multixact_offset_buffers(int *newval, void **extra,
										   GucSource source);
extern bool check_notify_buffers(int *newval, void **extra, GucSource source);
extern bool check_primary_slot_name(char **newval, void **extra,
									GucSource source);
extern bool check_random_seed(double *newval, void **extra, GucSource source);
