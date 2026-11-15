// AUTO-DRAFT from torvalds/linux PR #23c62304dde29abe92013d85910d03a77dada35d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void gntdev_print_maps(struct gntdev_priv *priv,
			      char *text, int text_index)
{
#ifdef DEBUG
	struct gntdev_grant_map *map;

	pr_debug("%s: maps list (priv %p)\n", __func__, priv);
	list_for_each_entry(map, &priv->maps, next)
		pr_debug("  index %2d, count %2d %s\n",
		       map->index, map->count,
		       map->index == text_index && text ? text : "");
#endif
}
/* …（同文件无关代码省略）… */
static void gntdev_free_map(struct gntdev_grant_map *map)
{
	if (map == NULL)
		return;

#ifdef CONFIG_XEN_GRANT_DMA_ALLOC
	if (map->dma_vaddr) {
		struct gnttab_dma_alloc_args args;

		args.dev = map->dma_dev;
		args.coherent = !!(map->dma_flags & GNTDEV_DMA_FLAG_COHERENT);
		args.nr_pages = map->count;
		args.pages = map->pages;
		args.frames = map->frames;
		args.vaddr = map->dma_vaddr;
		args.dev_bus_addr = map->dma_bus_addr;

		gnttab_dma_free_pages(&args);
	} else
#endif
	if (map->pages)
		gnttab_free_pages(map->count, map->pages);

#ifdef CONFIG_XEN_GRANT_DMA_ALLOC
	kvfree(map->frames);
#endif
	kvfree(map->pages);
	kvfree(map->grants);
	kvfree(map->map_ops);
	kvfree(map->unmap_ops);
	kvfree(map->kmap_ops);
	kvfree(map->kunmap_ops);
	kvfree(map->being_removed);
	kfree(map);
}
/* …（同文件无关代码省略）… */
void gntdev_add_map(struct gntdev_priv *priv, struct gntdev_grant_map *add)
{
	struct gntdev_grant_map *map;

	list_for_each_entry(map, &priv->maps, next) {
		if (add->index + add->count < map->index) {
			list_add_tail(&add->next, &map->next);
			goto done;
		}
		add->index = map->index + map->count;
	}
	list_add_tail(&add->next, &priv->maps);

done:
	gntdev_print_maps(priv, "[new]", add->index);
}
/* …（同文件无关代码省略）… */
void gntdev_put_map(struct gntdev_priv *priv, struct gntdev_grant_map *map)
{
	if (!map)
		return;

	if (!refcount_dec_and_test(&map->users))
		return;

	if (map->pages && !xen_pv_domain()) {
		/*
		 * Increment the reference count.  This ensures that the
		 * subsequent call to unmap_grant_pages() will not wind up
		 * re-entering itself.  It *can* wind up calling
		 * gntdev_put_map() recursively, but such calls will be with a
		 * reference count greater than 1, so they will return before
		 * this code is reached.  The recursion depth is thus limited to
		 * 1.  Do NOT use refcount_inc() here, as it will detect that
		 * the reference count is zero and WARN().
		 */
		refcount_set(&map->users, 1);

		/*
		 * Unmap the grants.  This may or may not be asynchronous, so it
		 * is possible that the reference count is 1 on return, but it
		 * could also be greater than 1.
		 */
		unmap_grant_pages(map, 0, map->count);

		/* Check if the memory now needs to be freed */
		if (!refcount_dec_and_test(&map->users))
			return;

		/*
		 * All pages have been returned to the hypervisor, so free the
		 * map.
		 */
	}

	if (xen_pv_domain() && map->notifier_init)
		mmu_interval_notifier_remove(&map->notifier);

	if (map->notify.flags & UNMAP_NOTIFY_SEND_EVENT) {
		notify_remote_via_evtchn(map->notify.event);
		evtchn_put(map->notify.event);
	}
	gntdev_free_map(map);
}
/* …（同文件无关代码省略）… */
static void __unmap_grant_pages_done(int result,
		struct gntab_unmap_queue_data *data)
{
	unsigned int i;
	struct gntdev_grant_map *map = data->data;
	unsigned int offset = data->unmap_ops - map->unmap_ops;
	int successful_unmaps = 0;
	int live_grants;

	for (i = 0; i < data->count; i++) {
		if (map->unmap_ops[offset + i].status == GNTST_okay &&
		    map->unmap_ops[offset + i].handle != INVALID_GRANT_HANDLE)
			successful_unmaps++;

		WARN_ON(map->unmap_ops[offset + i].status != GNTST_okay &&
			map->unmap_ops[offset + i].handle != INVALID_GRANT_HANDLE);
		pr_debug("unmap handle=%d st=%d\n",
			map->unmap_ops[offset+i].handle,
			map->unmap_ops[offset+i].status);
		map->unmap_ops[offset+i].handle = INVALID_GRANT_HANDLE;
		if (xen_pv_domain()) {
			if (map->kunmap_ops[offset + i].status == GNTST_okay &&
			    map->kunmap_ops[offset + i].handle != INVALID_GRANT_HANDLE)
				successful_unmaps++;

			WARN_ON(map->kunmap_ops[offset + i].status != GNTST_okay &&
				map->kunmap_ops[offset + i].handle != INVALID_GRANT_HANDLE);
			pr_debug("kunmap handle=%u st=%d\n",
				 map->kunmap_ops[offset+i].handle,
				 map->kunmap_ops[offset+i].status);
			map->kunmap_ops[offset+i].handle = INVALID_GRANT_HANDLE;
		}
	}

	/*
	 * Decrease the live-grant counter.  This must happen after the loop to
	 * prevent premature reuse of the grants by gnttab_mmap().
	 */
	live_grants = atomic_sub_return(successful_unmaps, &map->live_grants);
	if (WARN_ON(live_grants < 0))
		pr_err("%s: live_grants became negative (%d) after unmapping %d pages!\n",
		       __func__, live_grants, successful_unmaps);

	/* Release reference taken by __unmap_grant_pages */
	gntdev_put_map(NULL, map);
}
/* …（同文件无关代码省略）… */
static void __unmap_grant_pages(struct gntdev_grant_map *map, int offset,
			       int pages)
{
	if (map->notify.flags & UNMAP_NOTIFY_CLEAR_BYTE) {
		int pgno = (map->notify.addr >> PAGE_SHIFT);

		if (pgno >= offset && pgno < offset + pages) {
			/* No need for kmap, pages are in lowmem */
			uint8_t *tmp = pfn_to_kaddr(page_to_pfn(map->pages[pgno]));

			tmp[map->notify.addr & (PAGE_SIZE-1)] = 0;
			map->notify.flags &= ~UNMAP_NOTIFY_CLEAR_BYTE;
		}
	}

	map->unmap_data.unmap_ops = map->unmap_ops + offset;
	map->unmap_data.kunmap_ops = xen_pv_domain() ? map->kunmap_ops + offset : NULL;
	map->unmap_data.pages = map->pages + offset;
	map->unmap_data.count = pages;
	map->unmap_data.done = __unmap_grant_pages_done;
	map->unmap_data.data = map;
	refcount_inc(&map->users); /* to keep map alive during async call below */

	gnttab_unmap_refs_async(&map->unmap_data);
}
/* …（同文件无关代码省略）… */
static void unmap_grant_pages(struct gntdev_grant_map *map, int offset,
			      int pages)
{
	int range;

	if (atomic_read(&map->live_grants) == 0)
		return; /* Nothing to do */

	pr_debug("unmap %d+%d [%d+%d]\n", map->index, map->count, offset, pages);

	/* It is possible the requested range will have a "hole" where we
	 * already unmapped some of the grants. Only unmap valid ranges.
	 */
	while (pages) {
		while (pages && map->being_removed[offset]) {
			offset++;
			pages--;
		}
		range = 0;
		while (range < pages) {
			if (map->being_removed[offset + range])
				break;
			map->being_removed[offset + range] = true;
			range++;
		}
		if (range)
			__unmap_grant_pages(map, offset, range);
		offset += range;
		pages -= range;
	}
}
/* …（同文件无关代码省略）… */
	mutex_lock(&priv->lock);
	gntdev_add_map(priv, map);
	op.index = map->index << PAGE_SHIFT;

	if (copy_to_user(u, &op, sizeof(op)) != 0) {
		list_del(&map->next);
		mutex_unlock(&priv->lock);
		gntdev_put_map(priv, map);
		return -EFAULT;
	}

	mutex_unlock(&priv->lock);
	return 0;
}
