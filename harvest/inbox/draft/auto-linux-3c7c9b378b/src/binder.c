// AUTO-DRAFT from torvalds/linux PR #25d576ed11108470d0999054265108400db1b881
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static __printf(2, 3) void binder_debug(int mask, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	if (binder_debug_mask & mask) {
		va_start(args, format);
		vaf.va = &args;
		vaf.fmt = format;
		pr_info_ratelimited("%pV", &vaf);
		va_end(args);
	}
}
/* …（同文件无关代码省略）… */
#define to_flat_binder_object(hdr) \
	container_of(hdr, struct flat_binder_object, hdr)
/* …（同文件无关代码省略）… */
#define to_binder_fd_array_object(hdr) \
	container_of(hdr, struct binder_fd_array_object, hdr)
/* …（同文件无关代码省略）… */
static inline void binder_stats_deleted(enum binder_stat_types type)
{
	atomic_inc(&binder_stats.obj_deleted[type]);
}
/* …（同文件无关代码省略）… */
#define binder_proc_lock(proc) _binder_proc_lock(proc, __LINE__)
/* …（同文件无关代码省略）… */
#define binder_proc_unlock(proc) _binder_proc_unlock(proc, __LINE__)
/* …（同文件无关代码省略）… */
#define binder_inner_proc_lock(proc) _binder_inner_proc_lock(proc, __LINE__)
/* …（同文件无关代码省略）… */
#define binder_inner_proc_unlock(proc) _binder_inner_proc_unlock(proc, __LINE__)
/* …（同文件无关代码省略）… */
#define binder_node_inner_lock(node) _binder_node_inner_lock(node, __LINE__)
/* …（同文件无关代码省略）… */
#define binder_node_inner_unlock(node) _binder_node_inner_unlock(node, __LINE__)
/* …（同文件无关代码省略）… */
static void
binder_enqueue_work_ilocked(struct binder_work *work,
			   struct list_head *target_list)
{
	BUG_ON(target_list == NULL);
	BUG_ON(work->entry.next && !list_empty(&work->entry));
	list_add_tail(&work->entry, target_list);
}
/* …（同文件无关代码省略）… */
static void
binder_dequeue_work_ilocked(struct binder_work *work)
{
	list_del_init(&work->entry);
}
/* …（同文件无关代码省略）… */
static void binder_wakeup_proc_ilocked(struct binder_proc *proc)
{
	struct binder_thread *thread = binder_select_thread_ilocked(proc);

	binder_wakeup_thread_ilocked(proc, thread, /* sync = */false);
}
/* …（同文件无关代码省略）… */
static struct binder_node *binder_get_node_ilocked(struct binder_proc *proc,
						   binder_uintptr_t ptr)
{
	struct rb_node *n = proc->nodes.rb_node;
	struct binder_node *node;

	assert_spin_locked(&proc->inner_lock);

	while (n) {
		node = rb_entry(n, struct binder_node, rb_node);

		if (ptr < node->ptr)
			n = n->rb_left;
		else if (ptr > node->ptr)
			n = n->rb_right;
		else {
			/*
			 * take an implicit weak reference
			 * to ensure node stays alive until
			 * call to binder_put_node()
			 */
			binder_inc_node_tmpref_ilocked(node);
			return node;
		}
	}
	return NULL;
}
/* …（同文件无关代码省略）… */
static struct binder_node *binder_get_node(struct binder_proc *proc,
					   binder_uintptr_t ptr)
{
	struct binder_node *node;

	binder_inner_proc_lock(proc);
	node = binder_get_node_ilocked(proc, ptr);
	binder_inner_proc_unlock(proc);
	return node;
}
/* …（同文件无关代码省略）… */
static void binder_free_node(struct binder_node *node)
{
	kfree(node);
	binder_stats_deleted(BINDER_STAT_NODE);
}
/* …（同文件无关代码省略）… */
static bool binder_dec_node_nilocked(struct binder_node *node,
				     int strong, int internal)
{
	struct binder_proc *proc = node->proc;

	assert_spin_locked(&node->lock);
	if (proc)
		assert_spin_locked(&proc->inner_lock);
	if (strong) {
		if (internal)
			node->internal_strong_refs--;
		else
			node->local_strong_refs--;
		if (node->local_strong_refs || node->internal_strong_refs)
			return false;
	} else {
		if (!internal)
			node->local_weak_refs--;
		if (node->local_weak_refs || node->tmp_refs ||
				!hlist_empty(&node->refs))
			return false;
	}

	if (proc && (node->has_strong_ref || node->has_weak_ref)) {
		if (list_empty(&node->work.entry)) {
			binder_enqueue_work_ilocked(&node->work, &proc->todo);
			binder_wakeup_proc_ilocked(proc);
		}
	} else {
		if (hlist_empty(&node->refs) && !node->local_strong_refs &&
		    !node->local_weak_refs && !node->tmp_refs) {
			if (proc) {
				binder_dequeue_work_ilocked(&node->work);
				rb_erase(&node->rb_node, &proc->nodes);
				binder_debug(BINDER_DEBUG_INTERNAL_REFS,
					     "refless node %d deleted\n",
					     node->debug_id);
			} else {
				BUG_ON(!list_empty(&node->work.entry));
				spin_lock(&binder_dead_nodes_lock);
				/*
				 * tmp_refs could have changed so
				 * check it again
				 */
				if (node->tmp_refs) {
					spin_unlock(&binder_dead_nodes_lock);
					return false;
				}
				hlist_del(&node->dead_node);
				spin_unlock(&binder_dead_nodes_lock);
				binder_debug(BINDER_DEBUG_INTERNAL_REFS,
					     "dead node %d deleted\n",
					     node->debug_id);
			}
			return true;
		}
	}
	return false;
}
/* …（同文件无关代码省略）… */
static void binder_dec_node(struct binder_node *node, int strong, int internal)
{
	bool free_node;

	binder_node_inner_lock(node);
	free_node = binder_dec_node_nilocked(node, strong, internal);
	binder_node_inner_unlock(node);
	if (free_node)
		binder_free_node(node);
}
/* …（同文件无关代码省略）… */
static void binder_inc_node_tmpref_ilocked(struct binder_node *node)
{
	/*
	 * No call to binder_inc_node() is needed since we
	 * don't need to inform userspace of any changes to
	 * tmp_refs
	 */
	node->tmp_refs++;
}
/* …（同文件无关代码省略）… */
static void binder_dec_node_tmpref(struct binder_node *node)
{
	bool free_node;

	binder_node_inner_lock(node);
	if (!node->proc)
		spin_lock(&binder_dead_nodes_lock);
	else
		__acquire(&binder_dead_nodes_lock);
	node->tmp_refs--;
	BUG_ON(node->tmp_refs < 0);
	if (!node->proc)
		spin_unlock(&binder_dead_nodes_lock);
	else
		__release(&binder_dead_nodes_lock);
	/*
	 * Call binder_dec_node() to check if all refcounts are 0
	 * and cleanup is needed. Calling with strong=0 and internal=1
	 * causes no actual reference to be released in binder_dec_node().
	 * If that changes, a change is needed here too.
	 */
	free_node = binder_dec_node_nilocked(node, 0, 1);
	binder_node_inner_unlock(node);
	if (free_node)
		binder_free_node(node);
}
/* …（同文件无关代码省略）… */
static void binder_put_node(struct binder_node *node)
{
	binder_dec_node_tmpref(node);
}
/* …（同文件无关代码省略）… */
static struct binder_ref *binder_get_ref_olocked(struct binder_proc *proc,
						 u32 desc, bool need_strong_ref)
{
	struct rb_node *n = proc->refs_by_desc.rb_node;
	struct binder_ref *ref;

	while (n) {
		ref = rb_entry(n, struct binder_ref, rb_node_desc);

		if (desc < ref->data.desc) {
			n = n->rb_left;
		} else if (desc > ref->data.desc) {
			n = n->rb_right;
		} else if (need_strong_ref && !ref->data.strong) {
			binder_user_error("tried to use weak ref as strong ref\n");
			return NULL;
		} else {
			return ref;
		}
	}
	return NULL;
}
/* …（同文件无关代码省略）… */
static int binder_inc_ref_olocked(struct binder_ref *ref, int strong,
				  struct list_head *target_list)
{
	int ret;

	if (strong) {
		if (ref->data.strong == 0) {
			ret = binder_inc_node(ref->node, 1, 1, target_list);
			if (ret)
				return ret;
		}
		ref->data.strong++;
	} else {
		if (ref->data.weak == 0) {
			ret = binder_inc_node(ref->node, 0, 1, target_list);
			if (ret)
				return ret;
		}
		ref->data.weak++;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static bool binder_dec_ref_olocked(struct binder_ref *ref, int strong)
{
	if (strong) {
		if (ref->data.strong == 0) {
			binder_user_error("%d invalid dec strong, ref %d desc %d s %d w %d\n",
					  ref->proc->pid, ref->data.debug_id,
					  ref->data.desc, ref->data.strong,
					  ref->data.weak);
			return false;
		}
		ref->data.strong--;
		if (ref->data.strong == 0)
			binder_dec_node(ref->node, strong, 1);
	} else {
		if (ref->data.weak == 0) {
			binder_user_error("%d invalid dec weak, ref %d desc %d s %d w %d\n",
					  ref->proc->pid, ref->data.debug_id,
					  ref->data.desc, ref->data.strong,
					  ref->data.weak);
			return false;
		}
		ref->data.weak--;
	}
	if (ref->data.strong == 0 && ref->data.weak == 0) {
		binder_cleanup_ref_olocked(ref);
		return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
static void binder_free_ref(struct binder_ref *ref)
{
	if (ref->node)
		binder_free_node(ref->node);
	kfree(ref->death);
	kfree(ref->freeze);
	kfree(ref);
}
/* …（同文件无关代码省略）… */
static void try_shrink_dmap(struct binder_proc *proc)
{
	unsigned long *new;
	int nbits;

	binder_proc_lock(proc);
	nbits = dbitmap_shrink_nbits(&proc->dmap);
	binder_proc_unlock(proc);

	if (!nbits)
		return;

	new = bitmap_zalloc(nbits, GFP_KERNEL);
	binder_proc_lock(proc);
	dbitmap_shrink(&proc->dmap, new, nbits);
	binder_proc_unlock(proc);
}
/* …（同文件无关代码省略）… */
static int binder_update_ref_for_handle(struct binder_proc *proc,
		uint32_t desc, bool increment, bool strong,
		struct binder_ref_data *rdata)
{
	int ret = 0;
	struct binder_ref *ref;
	bool delete_ref = false;

	binder_proc_lock(proc);
	ref = binder_get_ref_olocked(proc, desc, strong);
	if (!ref) {
		ret = -EINVAL;
		goto err_no_ref;
	}
	if (increment)
		ret = binder_inc_ref_olocked(ref, strong, NULL);
	else
		delete_ref = binder_dec_ref_olocked(ref, strong);

	if (rdata)
		*rdata = ref->data;
	binder_proc_unlock(proc);

	if (delete_ref) {
		binder_free_ref(ref);
		try_shrink_dmap(proc);
	}
	return ret;

err_no_ref:
	binder_proc_unlock(proc);
	return ret;
}
/* …（同文件无关代码省略）… */
static int binder_dec_ref_for_handle(struct binder_proc *proc,
		uint32_t desc, bool strong, struct binder_ref_data *rdata)
{
	return binder_update_ref_for_handle(proc, desc, false, strong, rdata);
}
/* …（同文件无关代码省略）… */
static size_t binder_get_object(struct binder_proc *proc,
				const void __user *u,
				struct binder_buffer *buffer,
				unsigned long offset,
				struct binder_object *object)
{
	size_t read_size;
	struct binder_object_header *hdr;
	size_t object_size = 0;

	read_size = min_t(size_t, sizeof(*object), buffer->data_size - offset);
	if (offset > buffer->data_size || read_size < sizeof(*hdr) ||
	    !IS_ALIGNED(offset, sizeof(u32)))
		return 0;

	if (u) {
		if (copy_from_user(object, u + offset, read_size))
			return 0;
	} else {
		if (binder_alloc_copy_from_buffer(&proc->alloc, object, buffer,
						  offset, read_size))
			return 0;
	}

	/* Ok, now see if we read a complete object. */
	hdr = &object->hdr;
	switch (hdr->type) {
	case BINDER_TYPE_BINDER:
	case BINDER_TYPE_WEAK_BINDER:
	case BINDER_TYPE_HANDLE:
	case BINDER_TYPE_WEAK_HANDLE:
		object_size = sizeof(struct flat_binder_object);
		break;
	case BINDER_TYPE_FD:
		object_size = sizeof(struct binder_fd_object);
		break;
	case BINDER_TYPE_PTR:
		object_size = sizeof(struct binder_buffer_object);
		break;
	case BINDER_TYPE_FDA:
		object_size = sizeof(struct binder_fd_array_object);
		break;
	default:
		return 0;
	}
	if (offset <= buffer->data_size - object_size &&
	    buffer->data_size >= object_size)
		return object_size;
	else
		return 0;
}
/* …（同文件无关代码省略）… */
static struct binder_buffer_object *binder_validate_ptr(
						struct binder_proc *proc,
						struct binder_buffer *b,
						struct binder_object *object,
						binder_size_t index,
						binder_size_t start_offset,
						binder_size_t *object_offsetp,
						binder_size_t num_valid)
{
	size_t object_size;
	binder_size_t object_offset;
	unsigned long buffer_offset;

	if (index >= num_valid)
		return NULL;

	buffer_offset = start_offset + sizeof(binder_size_t) * index;
	if (binder_alloc_copy_from_buffer(&proc->alloc, &object_offset,
					  b, buffer_offset,
					  sizeof(object_offset)))
		return NULL;
	object_size = binder_get_object(proc, NULL, b, object_offset, object);
	if (!object_size || object->hdr.type != BINDER_TYPE_PTR)
		return NULL;
	if (object_offsetp)
		*object_offsetp = object_offset;

	return &object->bbo;
}
/* …（同文件无关代码省略）… */
struct binder_task_work_cb {
	struct callback_head twork;
	struct file *file;
};
/* …（同文件无关代码省略）… */
static void binder_do_fd_close(struct callback_head *twork)
{
	struct binder_task_work_cb *twcb = container_of(twork,
			struct binder_task_work_cb, twork);

	fput(twcb->file);
	kfree(twcb);
}
/* …（同文件无关代码省略）… */
static void binder_deferred_fd_close(int fd)
{
	struct binder_task_work_cb *twcb;

	twcb = kzalloc_obj(*twcb);
	if (!twcb)
		return;
	init_task_work(&twcb->twork, binder_do_fd_close);
	twcb->file = file_close_fd(fd);
	if (twcb->file) {
		// pin it until binder_do_fd_close(); see comments there
		get_file(twcb->file);
		filp_close(twcb->file, current->files);
		task_work_add(current, &twcb->twork, TWA_RESUME);
	} else {
		kfree(twcb);
	}
}
/* …（同文件无关代码省略）… */
static void binder_transaction_buffer_release(struct binder_proc *proc,
					      struct binder_thread *thread,
					      struct binder_buffer *buffer,
					      binder_size_t off_end_offset,
					      bool is_failure)
{
	int debug_id = buffer->debug_id;
	binder_size_t off_start_offset, buffer_offset;

	binder_debug(BINDER_DEBUG_TRANSACTION,
		     "%d buffer release %d, size %zd-%zd, failed at %llx\n",
		     proc->pid, buffer->debug_id,
		     buffer->data_size, buffer->offsets_size,
		     (unsigned long long)off_end_offset);

	if (buffer->target_node)
		binder_dec_node(buffer->target_node, 1, 0);

	off_start_offset = ALIGN(buffer->data_size, sizeof(void *));

	for (buffer_offset = off_start_offset; buffer_offset < off_end_offset;
	     buffer_offset += sizeof(binder_size_t)) {
		struct binder_object_header *hdr;
		size_t object_size = 0;
		struct binder_object object;
		binder_size_t object_offset;

		if (!binder_alloc_copy_from_buffer(&proc->alloc, &object_offset,
						   buffer, buffer_offset,
						   sizeof(object_offset)))
			object_size = binder_get_object(proc, NULL, buffer,
							object_offset, &object);
		if (object_size == 0) {
			pr_err("transaction release %d bad object at offset %lld, size %zd\n",
			       debug_id, (u64)object_offset, buffer->data_size);
			continue;
		}
		hdr = &object.hdr;
		switch (hdr->type) {
		case BINDER_TYPE_BINDER:
		case BINDER_TYPE_WEAK_BINDER: {
			struct flat_binder_object *fp;
			struct binder_node *node;

			fp = to_flat_binder_object(hdr);
			node = binder_get_node(proc, fp->binder);
			if (node == NULL) {
				pr_err("transaction release %d bad node %016llx\n",
				       debug_id, (u64)fp->binder);
				break;
			}
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        node %d u%016llx\n",
				     node->debug_id, (u64)node->ptr);
			binder_dec_node(node, hdr->type == BINDER_TYPE_BINDER,
					0);
			binder_put_node(node);
		} break;
		case BINDER_TYPE_HANDLE:
		case BINDER_TYPE_WEAK_HANDLE: {
			struct flat_binder_object *fp;
			struct binder_ref_data rdata;
			int ret;

			fp = to_flat_binder_object(hdr);
			ret = binder_dec_ref_for_handle(proc, fp->handle,
				hdr->type == BINDER_TYPE_HANDLE, &rdata);

			if (ret) {
				pr_err("transaction release %d bad handle %d, ret = %d\n",
				 debug_id, fp->handle, ret);
				break;
			}
			binder_debug(BINDER_DEBUG_TRANSACTION,
				     "        ref %d desc %d\n",
				     rdata.debug_id, rdata.desc);
		} break;

		case BINDER_TYPE_FD: {
			/*
			 * No need to close the file here since user-space
			 * closes it for successfully delivered
			 * transactions. For transactions that weren't
			 * delivered, the new fd was never allocated so
			 * there is no need to close and the fput on the
			 * file is done when the transaction is torn
			 * down.
			 */
		} break;
		case BINDER_TYPE_PTR:
			/*
			 * Nothing to do here, this will get cleaned up when the
			 * transaction buffer gets freed
			 */
			break;
		case BINDER_TYPE_FDA: {
			struct binder_fd_array_object *fda;
			struct binder_buffer_object *parent;
			struct binder_object ptr_object;
			binder_size_t fda_offset;
			size_t fd_index;
			binder_size_t fd_buf_size;
			binder_size_t num_valid;

			if (is_failure) {
				/*
				 * The fd fixups have not been applied so no
				 * fds need to be closed.
				 */
				continue;
			}

			num_valid = (buffer_offset - off_start_offset) /
						sizeof(binder_size_t);
			fda = to_binder_fd_array_object(hdr);
			parent = binder_validate_ptr(proc, buffer, &ptr_object,
						     fda->parent,
						     off_start_offset,
						     NULL,
						     num_valid);
			if (!parent) {
				pr_err("transaction release %d bad parent offset\n",
				       debug_id);
				continue;
			}
			fd_buf_size = sizeof(u32) * fda->num_fds;
			if (fda->num_fds >= SIZE_MAX / sizeof(u32)) {
				pr_err("transaction release %d invalid number of fds (%lld)\n",
				       debug_id, (u64)fda->num_fds);
				continue;
			}
			if (fd_buf_size > parent->length ||
			    fda->parent_offset > parent->length - fd_buf_size) {
				/* No space for all file descriptors here. */
				pr_err("transaction release %d not enough space for %lld fds in buffer\n",
				       debug_id, (u64)fda->num_fds);
				continue;
			}
			/*
			 * the source data for binder_buffer_object is visible
			 * to user-space and the @buffer element is the user
			 * pointer to the buffer_object containing the fd_array.
			 * Convert the address to an offset relative to
			 * the base of the transaction buffer.
			 */
			fda_offset = parent->buffer - buffer->user_data +
				fda->parent_offset;
			for (fd_index = 0; fd_index < fda->num_fds;
			     fd_index++) {
				u32 fd;
				int err;
				binder_size_t offset = fda_offset +
					fd_index * sizeof(fd);

				err = binder_alloc_copy_from_buffer(
						&proc->alloc, &fd, buffer,
						offset, sizeof(fd));
				WARN_ON(err);
				if (!err) {
					binder_deferred_fd_close(fd);
					/*
					 * Need to make sure the thread goes
					 * back to userspace to complete the
					 * deferred close
					 */
					if (thread)
						thread->looper_need_return = true;
				}
			}
		} break;
		default:
			pr_err("transaction release %d bad object type %x\n",
				debug_id, hdr->type);
			break;
		}
	}
}
/* …（同文件无关代码省略）… */
static inline void binder_release_entire_buffer(struct binder_proc *proc,
						struct binder_thread *thread,
						struct binder_buffer *buffer,
						bool is_failure)
{
	binder_size_t off_end_offset;

	off_end_offset = ALIGN(buffer->data_size, sizeof(void *));
	off_end_offset += buffer->offsets_size;

	binder_transaction_buffer_release(proc, thread, buffer,
					  off_end_offset, is_failure);
}
/* …（同文件无关代码省略）… */
		t_outdated->buffer = NULL;
		buffer->transaction = NULL;
		trace_binder_transaction_update_buffer_release(buffer);
		binder_release_entire_buffer(proc, NULL, buffer, false);
		binder_alloc_free_buf(&proc->alloc, buffer);
		kfree(t_outdated);
		binder_stats_deleted(BINDER_STAT_TRANSACTION);
	}
