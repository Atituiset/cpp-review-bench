// AUTO-DRAFT from torvalds/linux PR #0fcb05b72344f40806cbe810de8872f1698bf053
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>

#define poly1305_blocks		poly1305_blocks_generic
#define poly1305_emit		poly1305_emit_generic
/* …（同文件无关代码省略）… */
void poly1305_final(struct poly1305_desc_ctx *desc, u8 *dst)
{
	if (unlikely(desc->buflen)) {
		desc->buf[desc->buflen++] = 1;
		memset(desc->buf + desc->buflen, 0,
		       POLY1305_BLOCK_SIZE - desc->buflen);
		poly1305_blocks(&desc->state, desc->buf, POLY1305_BLOCK_SIZE,
				0);
	}

	poly1305_emit(&desc->state.h, dst, desc->s);
	*desc = (struct poly1305_desc_ctx){};
}
EXPORT_SYMBOL(poly1305_final);
