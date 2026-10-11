// AUTO-DRAFT from torvalds/linux PR #bb934c29fda8dcd409145a0deddc1a8095fec930
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		/* first get metadata_size by buffer = NULL */
		r = amdgpu_bo_get_metadata(bo, NULL, 0,
					   metadata_size, NULL);

		/* user buf_size is bigger than bo metadata_size
		 * allocate a buf at kernel space and copy */
		if (*metadata_size <= buffer_size) {
			*metadata_buffer = kzalloc(*metadata_size, GFP_KERNEL);

			if (!*metadata_buffer)
				return -ENOMEM;

			r = amdgpu_bo_get_metadata(bo, *metadata_buffer, *metadata_size,
						   NULL, &metadata_flags);
