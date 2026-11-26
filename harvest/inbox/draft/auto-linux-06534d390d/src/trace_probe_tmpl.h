// AUTO-DRAFT from torvalds/linux PR #2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static nokprobe_inline void
fetch_store_raw(unsigned long val, struct fetch_insn *code, void *buf)
{
	switch (code->size) {
	case 1:
		*(u8 *)buf = (u8)val;
		break;
	case 2:
		*(u16 *)buf = (u16)val;
		break;
	case 4:
		*(u32 *)buf = (u32)val;
		break;
	case 8:
		//TBD: 32bit signed
		*(u64 *)buf = (u64)val;
		break;
	default:
		*(unsigned long *)buf = val;
	}
}
/* …（同文件无关代码省略）… */
static nokprobe_inline int
fetch_store_symstrlen(unsigned long addr)
{
	char namebuf[KSYM_SYMBOL_LEN];
	int ret;

	ret = sprint_symbol(namebuf, addr);
	if (ret < 0)
		return 0;

	return ret + 1;
}
/* …（同文件无关代码省略）… */
static nokprobe_inline int
fetch_store_symstring(unsigned long addr, void *dest, void *base)
{
	char namebuf[KSYM_SYMBOL_LEN];
	int maxlen = get_loc_len(*(u32 *)dest);
	void *__dest;
	int len;

	if (unlikely(!maxlen))
		return -ENOMEM;

	__dest = get_loc_data(dest, base);
	len = sprint_symbol(namebuf, addr);
	if (len < 0) {
		*(u32 *)dest = make_data_loc(0, __dest - base);
		return 0;
	}

	len = min(len, maxlen - 1);
	memcpy(__dest, namebuf, len);
	((char *)__dest)[len] = '\0';
	*(u32 *)dest = make_data_loc(len + 1, __dest - base);

	return len + 1;
}
/* …（同文件无关代码省略）… */
	return 0;
}

/* From the 2nd stage, routine is same */
static nokprobe_inline int
process_fetch_insn_bottom(struct fetch_insn *code, unsigned long val,
			   void *dest, void *base)
{
	struct fetch_insn *s3 = NULL;
	int total = 0, ret = 0, i = 0;
	u32 loc = 0;
	unsigned long lval, llval = val;

stage2:
	/* 2nd stage: dereference memory if needed */
	do {
		lval = val;
		switch (code->op) {
		case FETCH_OP_DEREF:
			ret = probe_mem_read(&val, (void *)val + code->offset,
					     sizeof(val));
			break;
		case FETCH_OP_UDEREF:
			ret = probe_mem_read_user(&val,
				 (void *)val + code->offset, sizeof(val));
			break;
		case FETCH_OP_CPU_PTR:
			val = (unsigned long)this_cpu_ptr((void __percpu *)val);
			ret = 0;
			break;
		default:
			lval = llval;
			goto out;
		}
		if (ret)
			return ret;
		llval = lval;
		code++;
	} while (1);
/* …（同文件无关代码省略）… */
stage3:
	/* 3rd stage: store value to buffer */
	if (unlikely(!dest)) {
		switch (code->op) {
		case FETCH_OP_ST_STRING:
			ret = fetch_store_strlen(val + code->offset);
			code++;
			goto array;
		case FETCH_OP_ST_USTRING:
			ret = fetch_store_strlen_user(val + code->offset);
			code++;
			goto array;
		case FETCH_OP_ST_SYMSTR:
			ret = fetch_store_symstrlen(val + code->offset);
			code++;
			goto array;
		default:
			return -EILSEQ;
		}
	}

	switch (code->op) {
	case FETCH_OP_ST_RAW:
		fetch_store_raw(val, code, dest);
		break;
	case FETCH_OP_ST_MEM:
		probe_mem_read(dest, (void *)val + code->offset, code->size);
		break;
	case FETCH_OP_ST_UMEM:
		probe_mem_read_user(dest, (void *)val + code->offset, code->size);
		break;
	case FETCH_OP_ST_STRING:
		loc = *(u32 *)dest;
		ret = fetch_store_string(val + code->offset, dest, base);
		break;
	case FETCH_OP_ST_USTRING:
		loc = *(u32 *)dest;
		ret = fetch_store_string_user(val + code->offset, dest, base);
		break;
	case FETCH_OP_ST_SYMSTR:
		loc = *(u32 *)dest;
		ret = fetch_store_symstring(val + code->offset, dest, base);
		break;
	default:
		return -EILSEQ;
/* …（同文件无关代码省略）… */
			if (s3->op != FETCH_OP_ST_STRING &&
			    s3->op != FETCH_OP_ST_USTRING) {
				dest += s3->size;
				val += s3->size;
				goto stage3;
			}
			code--;
			val = lval + sizeof(char *);
			if (dest) {
				dest += sizeof(u32);
				*(u32 *)dest = update_data_loc(loc, ret);
