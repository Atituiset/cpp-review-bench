// AUTO-DRAFT from torvalds/linux PR #c44104f3642da720164042623e4832836aefdd8e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
#include <stdio.h>
  // <<< BUG ANCHOR
struct vmwgfx_gmrid_man {
	struct ttm_resource_manager manager;
	spinlock_t lock;
	struct ida gmr_ida;
	uint32_t max_gmr_ids;
	uint32_t max_gmr_pages;
	uint32_t used_gmr_pages;
	uint8_t type;
};
/* …（同文件无关代码省略）… */
static struct vmwgfx_gmrid_man *to_gmrid_manager(struct ttm_resource_manager *man)
{
	return container_of(man, struct vmwgfx_gmrid_man, manager);
}
/* …（同文件无关代码省略）… */
				  const struct ttm_place *place,
				  struct ttm_resource **res)
{
	struct vmwgfx_gmrid_man *gman = to_gmrid_manager(man);
	int id;

	*res = kmalloc_obj(**res);
	if (!*res)
/* …（同文件无关代码省略）… */
			const unsigned long max_graphics_pages = totalram_pages() / 2;
			uint32_t new_max_pages = 0;

			DRM_WARN("vmwgfx: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");
			vmw_host_printf("vmwgfx, warning: mob memory overflow. Consider increasing guest RAM and graphicsMemory.\n");

			if (gman->max_gmr_pages > (max_graphics_pages / 2)) {
				DRM_WARN("vmwgfx: guest requires more than half of RAM for graphics.\n");
				new_max_pages = max_graphics_pages;
			} else
				new_max_pages = gman->max_gmr_pages * 2;
			if (new_max_pages > gman->max_gmr_pages && new_max_pages >= gman->used_gmr_pages) {
				DRM_WARN("vmwgfx: increasing guest mob limits to %u KiB.\n",
					 ((new_max_pages) << (PAGE_SHIFT - 10)));

				gman->max_gmr_pages = new_max_pages;
			} else {
				char buf[256];
				snprintf(buf, sizeof(buf),
					 "vmwgfx, error: guest graphics is out of memory (mob limit at: %u KiB).\n",
					 ((gman->max_gmr_pages) << (PAGE_SHIFT - 10)));
				vmw_host_printf(buf);
				DRM_WARN("%s", buf);
				goto nospace;
			}
		}
	}

	(*res)->start = id;

	spin_unlock(&gman->lock);
	return 0;

nospace:
	gman->used_gmr_pages -= PFN_UP((*res)->size);
	spin_unlock(&gman->lock);
	ida_free(&gman->gmr_ida, id);
	ttm_resource_fini(man, *res);
	kfree(*res);
	return -ENOSPC;
}

static void vmw_gmrid_man_put_node(struct ttm_resource_manager *man,
				   struct ttm_resource *res)
{
	struct vmwgfx_gmrid_man *gman = to_gmrid_manager(man);

	ida_free(&gman->gmr_ida, res->start);
	spin_lock(&gman->lock);
	gman->used_gmr_pages -= PFN_UP(res->size);
	spin_unlock(&gman->lock);
	ttm_resource_fini(man, res);
	kfree(res);
}
