// AUTO-DRAFT from torvalds/linux PR #3b7cab693ba2bab63774bf5b988e8a61b2ef0f32
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
static void blkg_free_workfn(struct work_struct *work)
{
	struct blkcg_gq *blkg = container_of(work, struct blkcg_gq,
					     free_work);
	struct request_queue *q = blkg->q;
	int i;

	/*
	 * pd_free_fn() can also be called from blkcg_deactivate_policy(),
	 * in order to make sure pd_free_fn() is called in order, the deletion
	 * of the list blkg->q_node is delayed to here from blkg_destroy(), and
	 * blkcg_mutex is used to synchronize blkg_free_workfn() and
	 * blkcg_deactivate_policy().
	 */
	mutex_lock(&q->blkcg_mutex);
	for (i = 0; i < BLKCG_MAX_POLS; i++)
		if (blkg->pd[i])
			blkcg_policy[i]->pd_free_fn(blkg->pd[i]);
	if (blkg->parent)
		blkg_put(blkg->parent);
	spin_lock_irq(&q->queue_lock);
	list_del_init(&blkg->q_node);
	spin_unlock_irq(&q->queue_lock);
	mutex_unlock(&q->blkcg_mutex);

	/*
	 * Release blkcg css ref only after blkg is removed from q->blkg_list,
	 * so concurrent iterators won't see a blkg with a freed blkcg.
	 */
	css_put(&blkg->blkcg->css);
	blk_put_queue(q);
	free_percpu(blkg->iostat_cpu);
	percpu_ref_exit(&blkg->refcnt);
	kfree(blkg);
}
/* …（同文件无关代码省略）… */
static void blkg_free(struct blkcg_gq *blkg)
{
	if (!blkg)
		return;

	/*
	 * Both ->pd_free_fn() and request queue's release handler may
	 * sleep, so free us by scheduling one work func
	 */
	INIT_WORK(&blkg->free_work, blkg_free_workfn);
	schedule_work(&blkg->free_work);
}
/* …（同文件无关代码省略）… */
static void __blkg_release(struct rcu_head *rcu)
{
	struct blkcg_gq *blkg = container_of(rcu, struct blkcg_gq, rcu_head);

#ifdef CONFIG_BLK_CGROUP_PUNT_BIO
	WARN_ON(!bio_list_empty(&blkg->async_bios));
#endif

	blkg_free(blkg);
}
/* …（同文件无关代码省略）… */
static void blkg_release(struct percpu_ref *ref)
{
	struct blkcg_gq *blkg = container_of(ref, struct blkcg_gq, refcnt);
	struct blkcg *blkcg = blkg->blkcg;
	int cpu;

	/*
	 * Flush all the non-empty percpu lockless lists before releasing
	 * us, given these stat belongs to us.
	 *
	 * blkg_stat_lock is for serializing blkg stat update
	 */
	for_each_possible_cpu(cpu)
		__blkcg_rstat_flush(blkcg, cpu);

	call_rcu(&blkg->rcu_head, __blkg_release);
}
/* …（同文件无关代码省略）… */
static void blkg_async_bio_workfn(struct work_struct *work)
{
	struct blkcg_gq *blkg = container_of(work, struct blkcg_gq,
					     async_bio_work);
	struct bio_list bios = BIO_EMPTY_LIST;
	struct bio *bio;
	struct blk_plug plug;
	bool need_plug = false;

	/* as long as there are pending bios, @blkg can't go away */
	spin_lock(&blkg->async_bio_lock);
	bio_list_merge_init(&bios, &blkg->async_bios);
	spin_unlock(&blkg->async_bio_lock);

	/* start plug only when bio_list contains at least 2 bios */
	if (bios.head && bios.head->bi_next) {
		need_plug = true;
		blk_start_plug(&plug);
	}
	while ((bio = bio_list_pop(&bios)))
		submit_bio(bio);
	if (need_plug)
		blk_finish_plug(&plug);
}
/* …（同文件无关代码省略）… */
static inline struct blkcg *blkcg_parent(struct blkcg *blkcg)
{
	return css_to_blkcg(blkcg->css.parent);
}
/* …（同文件无关代码省略）… */
static struct blkcg_gq *blkg_alloc(struct blkcg *blkcg, struct gendisk *disk,
				   gfp_t gfp_mask)
{
	struct blkcg_gq *blkg;
	int i, cpu;

	/* alloc and init base part */
	blkg = kzalloc_node(sizeof(*blkg), gfp_mask, disk->queue->node);
	if (!blkg)
		return NULL;
	if (percpu_ref_init(&blkg->refcnt, blkg_release, 0, gfp_mask))
		goto out_free_blkg;
	blkg->iostat_cpu = alloc_percpu_gfp(struct blkg_iostat_set, gfp_mask);
	if (!blkg->iostat_cpu)
		goto out_exit_refcnt;
	if (!blk_get_queue(disk->queue))
		goto out_free_iostat;
	/* blkg holds a reference to blkcg */
	if (!css_tryget_online(&blkcg->css))
		goto out_put_queue;

	blkg->q = disk->queue;
	INIT_LIST_HEAD(&blkg->q_node);
	blkg->blkcg = blkcg;
	blkg->iostat.blkg = blkg;
#ifdef CONFIG_BLK_CGROUP_PUNT_BIO
	spin_lock_init(&blkg->async_bio_lock);
	bio_list_init(&blkg->async_bios);
	INIT_WORK(&blkg->async_bio_work, blkg_async_bio_workfn);
#endif

	u64_stats_init(&blkg->iostat.sync);
	for_each_possible_cpu(cpu) {
		u64_stats_init(&per_cpu_ptr(blkg->iostat_cpu, cpu)->sync);
		per_cpu_ptr(blkg->iostat_cpu, cpu)->blkg = blkg;
	}

	for (i = 0; i < BLKCG_MAX_POLS; i++) {
		struct blkcg_policy *pol = blkcg_policy[i];
		struct blkg_policy_data *pd;

		if (!blkcg_policy_enabled(disk->queue, pol))
			continue;

		/* alloc per-policy data and attach it to blkg */
		pd = pol->pd_alloc_fn(disk, blkcg, gfp_mask);
		if (!pd)
			goto out_free_pds;
		blkg->pd[i] = pd;
		pd->blkg = blkg;
		pd->plid = i;
		pd->online = false;
	}

	return blkg;

out_free_pds:
	while (--i >= 0)
		if (blkg->pd[i])
			blkcg_policy[i]->pd_free_fn(blkg->pd[i]);
	css_put(&blkcg->css);
out_put_queue:
	blk_put_queue(disk->queue);
out_free_iostat:
	free_percpu(blkg->iostat_cpu);
out_exit_refcnt:
	percpu_ref_exit(&blkg->refcnt);
out_free_blkg:
	kfree(blkg);
	return NULL;
}
/* …（同文件无关代码省略）… */
static struct blkcg_gq *blkg_create(struct blkcg *blkcg, struct gendisk *disk,
				    struct blkcg_gq *new_blkg)
{
	struct blkcg_gq *blkg;
	int i, ret;

	lockdep_assert_held(&disk->queue->queue_lock);

	/* request_queue is dying, do not create/recreate a blkg */
	if (blk_queue_dying(disk->queue)) {
		ret = -ENODEV;
		goto err_free_blkg;
	}

	/* allocate */
	if (!new_blkg) {
		new_blkg = blkg_alloc(blkcg, disk, GFP_NOWAIT);
		if (unlikely(!new_blkg)) {
			ret = -ENOMEM;
			goto err_free_blkg;
		}
	}
	blkg = new_blkg;

	/* link parent */
	if (blkcg_parent(blkcg)) {
		blkg->parent = blkg_lookup(blkcg_parent(blkcg), disk->queue);
		if (WARN_ON_ONCE(!blkg->parent)) {
			ret = -ENODEV;
			goto err_free_blkg;
		}
		blkg_get(blkg->parent);
	}

	/* invoke per-policy init */
	for (i = 0; i < BLKCG_MAX_POLS; i++) {
		struct blkcg_policy *pol = blkcg_policy[i];

		if (blkg->pd[i] && pol->pd_init_fn)
			pol->pd_init_fn(blkg->pd[i]);
	}

	/* insert */
	spin_lock(&blkcg->lock);
	ret = radix_tree_insert(&blkcg->blkg_tree, disk->queue->id, blkg);
	if (likely(!ret)) {
		hlist_add_head_rcu(&blkg->blkcg
