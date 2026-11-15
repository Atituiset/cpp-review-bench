// AUTO-DRAFT from torvalds/linux PR #23c62304dde29abe92013d85910d03a77dada35d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/io.h>
/* …（同文件无关代码省略）… */
struct unmap_refs_callback_data {
	struct completion completion;
	int result;
};
/* …（同文件无关代码省略）… */
void gnttab_pages_clear_private(int nr_pages, struct page **pages)
{
	int i;

	for (i = 0; i < nr_pages; i++) {
#if BITS_PER_LONG < 64
		if (page_private(pages[i]))
			kfree((void *)page_private(pages[i]));
#endif
		set_page_private(pages[i], 0);
	}
}
/* …（同文件无关代码省略）… */
	size_t size;
	int i, ret;

	if (args->nr_pages < 0 || args->nr_pages > (INT_MAX >> PAGE_SHIFT))
		return -ENOMEM;

/* …（同文件无关代码省略）… */
		xenmem_reservation_scrub_page(page);
	}

	xenmem_reservation_va_mapping_reset(args->nr_pages, args->pages);

	ret = xenmem_reservation_decrease(args->nr_pages, args->frames);
	if (ret != args->nr_pages) {
		pr_debug("Failed to decrease reservation for DMA buffer\n");
		ret = -EFAULT;
		goto fail;
	}
/* …（同文件无关代码省略）… */
	size_t size;
	int i, ret;

	gnttab_pages_clear_private(args->nr_pages, args->pages);

	for (i = 0; i < args->nr_pages; i++)
/* …（同文件无关代码省略）… */
	ret = xenmem_reservation_increase(args->nr_pages, args->frames);
	if (ret != args->nr_pages) {
		pr_debug("Failed to increase reservation for DMA buffer\n");
		ret = -EFAULT;
	} else {
		ret = 0;
	}

	xenmem_reservation_va_mapping_update(args->nr_pages, args->pages,
					     args->frames);

	size = args->nr_pages << PAGE_SHIFT;
	if (args->coherent)
		dma_free_coherent(args->dev, size,
/* …（同文件无关代码省略）… */
int gnttab_unmap_refs(struct gnttab_unmap_grant_ref *unmap_ops,
		      struct gnttab_unmap_grant_ref *kunmap_ops,
		      struct page **pages, unsigned int count)
{
	unsigned int i;
	int ret;

	ret = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, unmap_ops, count);
	if (ret)
		return ret;

	for (i = 0; i < count; i++)
		ClearPageForeign(pages[i]);

	return clear_foreign_p2m_mapping(unmap_ops, kunmap_ops, pages, count);
}
/* …（同文件无关代码省略）… */
#define GNTTAB_UNMAP_REFS_DELAY 5
/* …（同文件无关代码省略）… */
static void gnttab_unmap_work(struct work_struct *work)
{
	struct gntab_unmap_queue_data
		*unmap_data = container_of(work, 
					   struct gntab_unmap_queue_data,
					   gnttab_work.work);
	if (unmap_data->age != UINT_MAX)
		unmap_data->age++;
	__gnttab_unmap_refs_async(unmap_data);
}
/* …（同文件无关代码省略）… */
static void __gnttab_unmap_refs_async(struct gntab_unmap_queue_data* item)
{
	int ret;
	int pc;

	for (pc = 0; pc < item->count; pc++) {
		if (page_count(item->pages[pc]) > 1) {
			unsigned long delay = GNTTAB_UNMAP_REFS_DELAY * (item->age + 1);
			schedule_delayed_work(&item->gnttab_work,
					      msecs_to_jiffies(delay));
			return;
		}
	}

	ret = gnttab_unmap_refs(item->unmap_ops, item->kunmap_ops,
				item->pages, item->count);
	item->done(ret, item);
}
/* …（同文件无关代码省略）… */
void gnttab_unmap_refs_async(struct gntab_unmap_queue_data* item)
{
	INIT_DELAYED_WORK(&item->gnttab_work, gnttab_unmap_work);
	item->age = 0;

	__gnttab_unmap_refs_async(item);
}
/* …（同文件无关代码省略）… */
static void unmap_refs_callback(int result,
		struct gntab_unmap_queue_data *data)
{
	struct unmap_refs_callback_data *d = data->data;

	d->result = result;
	complete(&d->completion);
}
/* …（同文件无关代码省略）… */
int gnttab_unmap_refs_sync(struct gntab_unmap_queue_data *item)
{
	struct unmap_refs_callback_data data;

	init_completion(&data.completion);
	item->data = &data;
	item->done = &unmap_refs_callback;
	gnttab_unmap_refs_async(item);
	wait_for_completion(&data.completion);

	return data.result;
}
