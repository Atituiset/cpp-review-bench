// AUTO-DRAFT from torvalds/linux PR #b3593aba315322d875f295eef8b1a19e24658ba6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
			      unsigned long buf_len)
{
	const struct kexec_file_ops * const *fops;
	int ret = -ENOEXEC;

	for (fops = &kexec_file_loaders[0]; *fops && (*fops)->probe; ++fops) {
		ret = (*fops)->probe(buf, buf_len);
		if (!ret) {
			image->fops = *fops;
			return ret;
		}
	}

	return ret;
}

static void *kexec_image_load_default(struct kimage *image)
/* …（同文件无关代码省略）… */
{
	struct kimage *image = kbuf->image;
	unsigned long temp_start, temp_end;

	temp_end = min(end, kbuf->buf_max);
	temp_start = temp_end - kbuf->memsz + 1;
/* …（同文件无关代码省略）… */
		/* align down start */
		temp_start = ALIGN_DOWN(temp_start, kbuf->buf_align);

		if (temp_start < start || temp_start < kbuf->buf_min)
			return 0;

		temp_end = temp_start + kbuf->memsz - 1;
/* …（同文件无关代码省略）… */
			continue;
		}

		/* We found a suitable memory range */
		break;
	} while (1);
/* …（同文件无关代码省略）… */
{
	struct kimage *image = kbuf->image;
	unsigned long temp_start, temp_end;

	temp_start = max(start, kbuf->buf_min);

/* …（同文件无关代码省略）… */
			continue;
		}

		/* We found a suitable memory range */
		break;
	} while (1);
/* …（同文件无关代码省略）… */
	kbuf->cma = NULL;

	/* Walk the RAM ranges and allocate a suitable range for the buffer */
	ret = arch_kexec_locate_mem_hole(kbuf);
	if (ret)
		return ret;
