// AUTO-DRAFT from torvalds/linux PR #0f2732620d6ec49d3e6c06831575eaec6bc99d42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
		}
	}

	if (*ivmode && !strcmp(*ivmode, "lmk"))
		cc->tfms_count = 64;

	if (*ivmode && !strcmp(*ivmode, "essiv")) {
		if (!*ivopts) {
