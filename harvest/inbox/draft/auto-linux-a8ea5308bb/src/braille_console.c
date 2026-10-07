// AUTO-DRAFT from torvalds/linux PR #22430ae5d90ab288b0ee2ad99ae941f4a666b694
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
#define WIDTH 40
/* …（同文件无关代码省略）… */
{
	static u16 lastwrite[WIDTH];
	unsigned char data[1 + 1 + 2*WIDTH + 2 + 1], csum = 0, *c;
	u16 out;
	int i;

	if (!braille_co)
		return;

	if (!memcmp(lastwrite, buf, WIDTH * sizeof(*buf)))
		return;
	memcpy(lastwrite, buf, WIDTH * sizeof(*buf));

#define SOH 1
/* …（同文件无关代码省略）… */
#define ETX 2
/* …（同文件无关代码省略）… */
	*c++ = csum;
	*c++ = ETX;

	braille_co->write(braille_co, data, c - data);
}

/* Follow the VC cursor*/
