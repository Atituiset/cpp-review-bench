// AUTO-DRAFT from torvalds/linux PR #e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>


#define CS35L56_GPIO_FN_GPIO				0x00000001

/* Mixer input sources */
#define CS35L56_INPUT_SRC_NONE				0x00
#define CS35L56_INPUT_SRC_ASP1RX1			0x08
/* …（同文件无关代码省略）… */
int cs35l56_read_prot_status(struct cs35l56_base *cs35l56_base,
			     bool *fw_missing, unsigned int *fw_version);
void cs35l56_warn_if_firmware_missing(struct cs35l56_base *cs35l56_base);
void cs35l56_log_tuning(struct cs35l56_base *cs35l56_base, struct cs_dsp *cs_dsp);
int cs35l56_hw_init(struct cs35l56_base *cs35l56_base);
int cs35l56_get_speaker_id(struct cs35l56_base *cs35l56_base);
