// AUTO-DRAFT from torvalds/linux PR #54975c6523dc77862c25deeb5fa395990b8d3fc4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
struct aht10_data {
	struct i2c_client *client;
	ktime_t min_poll_interval;
	ktime_t previous_poll_time;
	int temperature;
	int humidity;
	bool crc8;
	unsigned int meas_size;
	u8 init_cmd;
};
/* …（同文件无关代码省略）… */
static ssize_t aht10_interval_write(struct aht10_data *data,
				    long val)
{
	data->min_poll_interval = ms_to_ktime(clamp_val(val, 2000, LONG_MAX));
	return 0;
}
