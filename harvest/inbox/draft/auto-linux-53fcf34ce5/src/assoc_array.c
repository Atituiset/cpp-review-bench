// AUTO-DRAFT from torvalds/linux PR #666539eecdf4db623f5e3c3e55cf9f2dfad24aa4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static int assoc_array_subtree_iterate(const struct assoc_array_ptr *root,
				       const struct assoc_array_ptr *stop,
				       int (*iterator)(const void *leaf,
						       void *iterator_data),
				       void *iterator_data)
{
	const struct assoc_array_shortcut *shortcut;
	const struct assoc_array_node *node;
	const struct assoc_array_ptr *cursor, *ptr, *parent;
	unsigned long has_meta;
	int slot, ret;

	cursor = root;

begin_node:
	if (assoc_array_ptr_is_shortcut(cursor)) {
		/* Descend through a shortcut */
		shortcut = assoc_array_ptr_to_shortcut(cursor);
		cursor = READ_ONCE(shortcut->next_node); /* Address dependency. */
	}

	node = assoc_array_ptr_to_node(cursor);
	slot = 0;

	/* We perform two passes of each node.
	 *
	 * The first pass does all the leaves in this node.  This means we
	 * don't miss any leaves if the node is split up by insertion whilst
	 * we're iterating over the branches rooted here (we may, however, see
	 * some leaves twice).
	 */
	has_meta = 0;
	for (; slot < ASSOC_ARRAY_FAN_OUT; slot++) {
		ptr = READ_ONCE(node->slots[slot]); /* Address dependency. */
		has_meta |= (unsigned long)ptr;
		if (ptr && assoc_array_ptr_is_leaf(ptr)) {
			/* We need a barrier between the read of the pointer,
			 * which is supplied by the above READ_ONCE().
			 */
			/* Invoke the callback */
			ret = iterator(assoc_array_ptr_to_leaf(ptr),
				       iterator_data);
			if (ret)
				return ret;
		}
	}

	/* The second pass attends to all the metadata pointers.  If we follow
	 * one of these we may find that we don't come back here, but rather go
	 * back to a replacement node with the leaves in a different layout.
	 *
	 * We are guaranteed to make progress, however, as the slot number for
	 * a particular portion of the key space cannot change - and we
	 * continue at the back pointer + 1.
	 */
	if (!(has_meta & ASSOC_ARRAY_PTR_META_TYPE))
		goto finished_node;
	slot = 0;

continue_node:
	node = assoc_array_ptr_to_node(cursor);
	for (; slot < ASSOC_ARRAY_FAN_OUT; slot++) {
		ptr = READ_ONCE(node->slots[slot]); /* Address dependency. */
		if (assoc_array_ptr_is_meta(ptr)) {
			cursor = ptr;
			goto begin_node;
		}
	}

finished_node:
	/* Move up to the parent (may need to skip back over a shortcut) */
	parent = READ_ONCE(node->back_pointer); /* Address dependency. */
	slot = node->parent_slot;
	if (parent == stop)
		return 0;

	if (assoc_array_ptr_is_shortcut(parent)) {
		shortcut = assoc_array_ptr_to_shortcut(parent);
		cursor = parent;
		parent = READ_ONCE(shortcut->back_pointer); /* Address dependency. */
		slot = shortcut->parent_slot;
		if (parent == stop)
			return 0;
	}

	/* Ascend to next slot in parent node */
	cursor = parent;
	slot++;
	goto continue_node;
}
/* …（同文件无关代码省略）… */
enum assoc_array_walk_status {
	assoc_array_walk_tree_empty,
	assoc_array_walk_found_terminal_node,
	assoc_array_walk_found_wrong_shortcut,
};
/* …（同文件无关代码省略）… */
struct assoc_array_walk_result {
	struct {
		struct assoc_array_node	*node;	/* Node in which leaf might be found */
		int		level;
		int		slot;
	} terminal_node;
	struct {
		struct assoc_array_shortcut *shortcut;
		int		level;
		int		sc_level;
		unsigned long	sc_segments;
		unsigned long	dissimilarity;
	} wrong_shortcut;
};
/* …（同文件无关代码省略）… */
static enum assoc_array_walk_status
assoc_array_walk(const struct assoc_array *array,
		 const struct assoc_array_ops *ops,
		 const void *index_key,
		 struct assoc_array_walk_result *result)
{
	struct assoc_array_shortcut *shortcut;
	struct assoc_array_node *node;
	struct assoc_array_ptr *cursor, *ptr;
	unsigned long sc_segments, dissimilarity;
	unsigned long segments;
	int level, sc_level, next_sc_level;
	int slot;

	pr_devel("-->%s()\n", __func__);

	cursor = READ_ONCE(array->root);  /* Address dependency. */
	if (!cursor)
		return assoc_array_walk_tree_empty;

	level = 0;

	/* Use segments from the key for the new leaf to navigate through the
	 * internal tree, skipping through nodes and shortcuts that are on
	 * route to the destination.  Eventually we'll come to a slot that is
	 * either empty or contains a leaf at which point we've found a node in
	 * which the leaf we're looking for might be found or into which it
	 * should be inserted.
	 */
jumped:
	segments = ops->get_key_chunk(index_key, level);
	pr_devel("segments[%d]: %lx\n", level, segments);

	if (assoc_array_ptr_is_shortcut(cursor))
		goto follow_shortcut;

consider_node:
	node = assoc_array_ptr_to_node(cursor);
	slot = segments >> (level & ASSOC_ARRAY_KEY_CHUNK_MASK);
	slot &= ASSOC_ARRAY_FAN_MASK;
	ptr = READ_ONCE(node->slots[slot]); /* Address dependency. */

	pr_devel("consider slot %x [ix=%d type=%lu]\n",
		 slot, level, (unsigned long)ptr & 3);

	if (!assoc_array_ptr_is_meta(ptr)) {
		/* The node doesn't have a node/shortcut pointer in the slot
		 * corresponding to the index key that we have to follow.
		 */
		result->terminal_node.node = node;
		result->terminal_node.level = level;
		result->terminal_node.slot = slot;
		pr_devel("<--%s() = terminal_node\n", __func__);
		return assoc_array_walk_found_terminal_node;
	}

	if (assoc_array_ptr_is_node(ptr)) {
		/* There is a pointer to a node in the slot corresponding to
		 * this index key segment, so we need to follow it.
		 */
		cursor = ptr;
		level += ASSOC_ARRAY_LEVEL_STEP;
		if ((level & ASSOC_ARRAY_KEY_CHUNK_MASK) != 0)
			goto consider_node;
		goto jumped;
	}

	/* There is a shortcut in the slot corresponding to the index key
	 * segment.  We follow the shortcut if its partial index key matches
	 * this leaf's.  Otherwise we need to split the shortcut.
	 */
	cursor = ptr;
follow_shortcut:
	shortcut = assoc_array_ptr_to_shortcut(cursor);
	pr_devel("shortcut to %d\n", shortcut->skip_to_level);
	sc_level = level + ASSOC_ARRAY_LEVEL_STEP;
	BUG_ON(sc_level > shortcut->skip_to_level);

	do {
		/* Check the leaf against the shortcut's index key a word at a
		 * time, trimming the final word (the shortcut stores the index
		 * key completely from the root to the shortcut's target).
		 */
		if ((sc_level & ASSOC_ARRAY_KEY_CHUNK_MASK) == 0)
			segments = ops->get_key_chunk(index_key, sc_level);

		sc_segments = shortcut->index_key[sc_level >> ASSOC_ARRAY_KEY_CHUNK_SHIFT];
		dissimilarity = segments ^ sc_segments;

		if (shortcut->skip_to_level < round_down(sc_level,
				ASSOC_ARRAY_KEY_CHUNK_SIZE) + ASSOC_ARRAY_KEY_CHUNK_SIZE) {
			/* Trim segments that are beyond the shortcut */
			int shift = shortcut->skip_to_level & ASSOC_ARRAY_KEY_CHUNK_MASK;
			dissimilarity &= ~(ULONG_MAX << shift);
			next_sc_level = shortcut->skip_to_level;
		} else {
			next_sc_level = sc_level + ASSOC_ARRAY_KEY_CHUNK_SIZE;
			next_sc_level = round_down(next_sc_level, ASSOC_ARRAY_KEY_CHUNK_SIZE);
		}

		if (dissimilarity != 0) {
			/* This shortcut points elsewhere */
			result->wrong_shortcut.shortcut = shortcut;
			result->wrong_shortcut.level = level;
			result->wrong_shortcut.sc_level = sc_level;
			result->wrong_shortcut.sc_segments = sc_segments;
			result->wrong_shortcut.dissimilarity = dissimilarity;
			return assoc_array_walk_found_wrong_shortcut;
		}

		sc_level = next_sc_level;
	} while (sc_level < shortcut->skip_to_level);

	/* The shortcut matches the leaf's index to this point. */
	cursor = READ_ONCE(shortcut->next_node); /* Address dependency. */
	if (((level ^ sc_level) & ~ASSOC_ARRAY_KEY_CHUNK_MASK) != 0) {
		level = sc_level;
		goto jumped;
	} else {
		level = sc_level;
		goto consider_node;
	}
}
/* …（同文件无关代码省略）… */
static void assoc_array_destroy_subtree(struct assoc_array_ptr *root,
					const struct assoc_array_ops *ops)
{
	struct assoc_array_shortcut *shortcut;
	struct assoc_array_node *node;
	struct assoc_array_ptr *cursor, *parent = NULL;
	int slot = -1;

	pr_devel("-->%s()\n", __func__);

	cursor = root;
	if (!cursor) {
		pr_devel("empty\n");
		return;
	}

move_to_meta:
	if (assoc_array_ptr_is_shortcut(cursor)) {
		/* Descend through a shortcut */
		pr_devel("[%d] shortcut\n", slot);
		BUG_ON(!assoc_array_ptr_is_shortcut(cursor));
		shortcut = assoc_array_ptr_to_shortcut(cursor);
		BUG_ON(shortcut->back_pointer != parent);
		BUG_ON(slot != -1 && shortcut->parent_slot != slot);
		parent = cursor;
		cursor = shortcut->next_node;
		slot = -1;
		BUG_ON(!assoc_array_ptr_is_node(cursor));
	}

	pr_devel("[%d] node\n", slot);
	node = assoc_array_ptr_to_node(cursor);
	BUG_ON(node->back_pointer != parent);
	BUG_ON(slot != -1 && node->parent_slot != slot);
	slot = 0;

continue_node:
	pr_devel("Node %p [back=%p]\n", node, node->back_pointer);
	for (; slot < ASSOC_ARRAY_FAN_OUT; slot++) {
		struct assoc_array_ptr *ptr = node->slots[slot];
		if (!ptr)
			continue;
		if (assoc_array_ptr_is_meta(ptr)) {
			parent = cursor;
			cursor = ptr;
			goto move_to_meta;
		}

		if (ops) {
			pr_devel("[%d] free leaf\n", slot);
			ops->free_object(assoc_array_ptr_to_leaf(ptr));
		}
	}

	parent = node->back_pointer;
	slot = node->parent_slot;
	pr_devel("free node\n");
	kfree(node);
	if (!parent)
		return; /* Done */

	/* Move back up to the parent (may need to free a shortcut on
	 * the way up) */
	if (assoc_array_ptr_is_shortcut(parent)) {
		shortcut = assoc_array_ptr_to_shortcut(parent);
		BUG_ON(shortcut->next_node != cursor);
		cursor = parent;
		parent = shortcut->back_pointer;
		slot = shortcut->parent_slot;
		pr_devel("free shortcut\n");
		kfree(shortcut);
		if (!parent)
			return;

		BUG_ON(!assoc_array_ptr_is_node(parent));
	}

	/* Ascend to next slot in parent node */
	pr_devel("ascend to %p[%d]\n", parent, slot);
	cursor = parent;
	node = assoc_array_ptr_to_node(cursor);
	slot++;
	goto continue_node;
}
/* …（同文件无关代码省略）… */
}

/*
 * Handle insertion into a terminal node.
 */
static bool assoc_array_insert_into_terminal_node(struct assoc_array_edit *edit,
						  const struct assoc_array_ops *ops,
						  const void *index_key,
						  struct assoc_array_walk_result *result)
{
	struct assoc_array_shortcut *shortcut, *new_s0;
	struct assoc_array_node *node, *new_n0, *new_n1, *side;
	struct assoc_array_ptr *ptr;
	unsigned long dissimilarity, base_seg, blank;
	size_t keylen;
	bool have_meta;
	int level, diff;
	int slot, next_slot, free_slot, i, j;

	node	= result->terminal_node.node;
	level	= result->terminal_node.level;
	edit->segment_cache[ASSOC_ARRAY_FAN_OUT] = result->terminal_node.slot;

	pr_devel("-->%s()\n", __func__);

	/* We arrived at a node which doesn't have an onward node or shortcut
	 * pointer that we have to follow.  This means that (a) the leaf we
	 * want must go here (either by insertion or replacement) or (b) we
	 * need to split this node and insert in one of the fragments.
	 */
	free_slot = -1;

	/* Firstly, we have to check the leaves in this node to see if there's
/* …（同文件无关代码省略）… */
			pr_devel("replace in slot %d\n", i);
			edit->leaf_p = &node->slots[i];
			edit->dead_leaf = node->slots[i];
			pr_devel("<--%s() = ok [replace]\n", __func__);
			return true;
		}
	}
/* …（同文件无关代码省略）… */
		pr_devel("insert in free slot %d\n", free_slot);
		edit->leaf_p = &node->slots[free_slot];
		edit->adjust_count_on = node;
		pr_devel("<--%s() = ok [insert]\n", __func__);
		return true;
	}

	/* The node has no spare slots - so we're either going to have to split
	 * it or insert another node before it.
	 *
	 * Whatever, we're going to need at least two new nodes - so allocate
	 * those now.  We may also need a new shortcut, but we deal with that
	 * when we need it.
	 */
	new_n0 = kzalloc_obj(struct assoc_array_node);
	if (!new_n0)
		return false;
	edit->new_meta[0] = assoc_array_node_to_ptr(new_n0);
	new_n1 = kzalloc_obj(struct assoc_array_node);
	if (!new_n1)
		return false;
	edit->new_meta[1] = assoc_array_node_to_ptr(new_n1);

	/* We need to find out how similar the leaves are. */
	pr_devel("no spare slots\n");
	have_meta = false;
	for (i = 0; i < ASSOC_ARRAY_FAN_OUT; i++) {
		ptr = node->slots[i];
/* …（同文件无关代码省略）… */
		base_seg >>= level & ASSOC_ARRAY_KEY_CHUNK_MASK;
		edit->segment_cache[i] = base_seg & ASSOC_ARRAY_FAN_MASK;
	}

	if (have_meta) {
		pr_devel("have meta\n");
		goto split_node;
	}

	/* The node contains only leaves */
	dissimilarity = 0;
	base_seg = edit->segment_cache[0];
	for (i = 1; i < ASSOC_ARRAY_FAN_OUT; i++)
		dissimilarity |= edit->segment_cache[i] ^ base_seg;

	pr_devel("only leaves; dissimilarity=%lx\n", dissimilarity);

	if ((dissimilarity & ASSOC_ARRAY_FAN_MASK) == 0) {
		/* The old leaves all cluster in the same slot.  We will need
		 * to insert a shortcut if the new node wants to cluster with them.
		 */
		if ((edit->segment_cache[ASSOC_ARRAY_FAN_OUT] ^ base_seg) == 0)
			goto all_leaves_cluster_together;

		/* Otherwise all the old leaves cluster in the same slot, but
		 * the new leaf wants to go into a different slot - so we
		 * create a new node (n0) to hold the new leaf and a pointer to
		 * a new node (n1) holding all the old leaves.
		 *
		 * This can be done by falling through to the node splitting
		 * path.
		 */
		pr_devel("present leaves cluster but not new leaf\n");
	}

split_node:
	pr_devel("split node\n");

	/* We need to split the current node.  The node must contain anything
	 * from a single leaf (in the one leaf case, this leaf will cluster
	 * with the new leaf) and the rest meta-pointers, to all leaves, some
	 * of which may cluster.
	 *
	 * It won't contain the case in which all the current leaves plus the
	 * new leaves want to cluster in the same slot.
	 *
	 * We need to expel at least two leaves out of a set consisting of the
	 * leaves in the node and the new leaf.  The current meta pointers can
	 * just be copied as they shouldn't cluster with any of the leaves.
	 *
	 * We need a new node (n0) to replace the current one and a new node to
	 * take the expelled nodes (n1).
	 */
	edit->set[0].to = assoc_array_node_to_ptr(new_n0);
	new_n0->back_pointer = node->back_pointer;
	new_n0->parent_slot = node->parent_slot;
	new_n1->back_pointer = assoc_array_node_to_ptr(new_n0);
	new_n1->parent_slot = -1; /* Need to calculate this */

do_split_node:
	pr_devel("do_split_node\n");

	new_n0->nr_leaves_on_branch = node->nr_leaves_on_branch;
	new_n1->nr_leaves_on_branch = 0;

/* …（同文件无关代码省略）… */
			}
		}
	}

	ptr = node->back_pointer;
	if (!ptr)
		edit->set[0].ptr = &edit->array->root;
	else if (assoc_array_ptr_is_node(ptr))
		edit->set[0].ptr = &assoc_array_ptr_to_node(ptr)->slots[node->parent_slot];
	else
		edit->set[0].ptr = &assoc_array_ptr_to_shortcut(ptr)->next_node;
	edit->excised_meta[0] = assoc_array_node_to_ptr(node);
	pr_devel("<--%s() = ok [split node]\n", __func__);
	return true;

all_leaves_cluster_together:
	/* All the leaves, new and old, want to cluster together in this node
	 * in the same slot, so we have to replace this node with a shortcut to
	 * skip over the identical parts of the key and then place a pair of
	 * nodes, one inside the other, at the end of the shortcut and
	 * distribute the keys between them.
	 *
	 * Firstly we need to work out where the leaves start diverging as a
	 * bit position into their keys so that we know how big the shortcut
	 * needs to be.
	 *
/* …（同文件无关代码省略）… */

	new_s0 = kzalloc_flex(*new_s0, index_key, keylen);
	if (!new_s0)
		return false;
	edit->new_meta[2] = assoc_array_shortcut_to_ptr(new_s0);

	edit->set[0].to = assoc_array_shortcut_to_ptr(new_s0);
	new_s0->back_pointer = node->back_pointer;
	new_s0->parent_slot = node->parent_slot;
	new_s0->next_node = assoc_array_node_to_ptr(new_n0);
	new_n0->back_pointer = assoc_array_shortcut_to_ptr(new_s0);
	new_n0->parent_slot = 0;
	new_n1->back_pointer = assoc_array_node_to_ptr(new_n0);
	new_n1->parent_slot = -1; /* Need to calculate this */

	new_s0->skip_to_level = level = diff & ~ASSOC_ARRAY_LEVEL_STEP_MASK;
	pr_devel("skip_to_level = %d [diff %d]\n", level, diff);
/* …（同文件无关代码省略）… */
	/* This now reduces to a node splitting exercise for which we'll need
	 * to regenerate the disparity table.
	 */
	for (i = 0; i < ASSOC_ARRAY_FAN_OUT; i++) {
		ptr = node->slots[i];
		base_seg = ops->get_object_key_chunk(assoc_array_ptr_to_leaf(ptr),
						     level);
		base_seg >>= level & ASSOC_ARRAY_KEY_CHUNK_MASK;
		edit->segment_cache[i] = base_seg & ASSOC_ARRAY_FAN_MASK;
	}

	base_seg = ops->get_key_chunk(index_key, level);
	base_seg >>= level & ASSOC_ARRAY_KEY_CHUNK_MASK;
	edit->segment_cache[ASSOC_ARRAY_FAN_OUT] = base_seg & ASSOC_ARRAY_FAN_MASK;
	goto do_split_node;
}

/*
/* …（同文件无关代码省略）… */
					    struct assoc_array_walk_result *result)
{
	struct assoc_array_shortcut *shortcut, *new_s0, *new_s1;
	struct assoc_array_node *node, *new_n0, *side;
	unsigned long sc_segments, dissimilarity, blank;
	size_t keylen;
	int level, sc_level, diff;
/* …（同文件无关代码省略）… */
	diff += sc_level & ~ASSOC_ARRAY_KEY_CHUNK_MASK;
	pr_devel("diff=%d\n", diff);

	if (!shortcut->back_pointer) {
		edit->set[0].ptr = &edit->array->root;
	} else if (assoc_array_ptr_is_node(shortcut->back_pointer)) {
		node = assoc_array_ptr_to_node(shortcut->back_pointer);
		edit->set[0].ptr = &node->slots[shortcut->parent_slot];
	} else {
		BUG();
	}

	edit->excised_meta[0] = assoc_array_shortcut_to_ptr(shortcut);

/* …（同文件无关代码省略）… */
	 * zero length - otherwise we just connect the new node directly to the
	 * parent.
	 */
	level += ASSOC_ARRAY_LEVEL_STEP;
	if (diff > level) {
		pr_devel("pre-shortcut %d...%d\n", level, diff);
		keylen = round_up(diff, ASSOC_ARRAY_KEY_CHUNK_SIZE);
/* …（同文件无关代码省略）… */
		memcpy(new_s0->index_key, shortcut->index_key,
		       flex_array_size(new_s0, index_key, keylen));

		blank = ULONG_MAX << (diff & ASSOC_ARRAY_KEY_CHUNK_MASK);
		pr_devel("blank off [%zu] %d: %lx\n", keylen - 1, diff, blank);
		new_s0->index_key[keylen - 1] &= ~blank;
	} else {
		pr_devel("no pre-shortcut\n");
		edit->set[0].to = assoc_array_node_to_ptr(new_n0);
/* …（同文件无关代码省略）… */
struct assoc_array_delete_collapse_context {
	struct assoc_array_node	*node;
	const void		*skip_leaf;
	int			slot;
};
/* …（同文件无关代码省略）… */
static int assoc_array_delete_collapse_iterator(const void *leaf,
						void *iterator_data)
{
	struct assoc_array_delete_collapse_context *collapse = iterator_data;

	if (leaf == collapse->skip_leaf)
		return 0;

	BUG_ON(collapse->slot >= ASSOC_ARRAY_FAN_OUT);

	collapse->node->slots[collapse->slot++] = assoc_array_leaf_to_ptr(leaf);
	return 0;
}

/**
 * assoc_array_delete - Script deletion of an object from an associative array
 * @array: The array to search.
/* …（同文件无关代码省略）… */
					    const struct assoc_array_ops *ops,
					    const void *index_key)
{
	struct assoc_array_delete_collapse_context collapse;
	struct assoc_array_walk_result result;
	struct assoc_array_node *node, *new_n0;
	struct assoc_array_edit *edit;
	struct assoc_array_ptr *ptr;
	bool has_meta;
	int slot, i;

	pr_devel("-->%s()\n", __func__);

/* …（同文件无关代码省略）… */
	edit->ops = ops;
	edit->adjust_count_by = -1;

	switch (assoc_array_walk(array, ops, index_key, &result)) {
	case assoc_array_walk_found_terminal_node:
		/* We found a node that should contain the leaf we've been
		 * asked to remove - *if* it's in the tree.
		 */
/* …（同文件无关代码省略）… */
			    assoc_array_ptr_is_leaf(ptr) &&
			    ops->compare_object(assoc_array_ptr_to_leaf(ptr),
						index_key))
				goto found_leaf;
		}
		fallthrough;
	case assoc_array_walk_tree_empty:
	case assoc_array_walk_found_wrong_shortcut:
	default:
		assoc_array_cancel_edit(edit);
		pr_devel("not found\n");
		return NULL;
	}

found_leaf:
	BUG_ON(array->nr_leaves_on_tree <= 0);

	/* In the simplest form of deletion we just clear the slot and release
/* …（同文件无关代码省略）… */
	 * We could also try and collapse in partially filled subtrees to take
	 * up space in this node.
	 */
	if (node->nr_leaves_on_branch <= ASSOC_ARRAY_FAN_OUT + 1) {
		struct assoc_array_node *parent, *grandparent;
		struct assoc_array_ptr *ptr;

		/* First of all, we need to know if this node has metadata so
		 * that we don't try collapsing if all the leaves are already
		 * here.
		 */
		has_meta = false;
		for (i = 0; i < ASSOC_ARRAY_FAN_OUT; i++) {
			ptr = node->slots[i];
			if (assoc_array_ptr_is_meta(ptr)) {
				has_meta = true;
				break;
			}
		}

		pr_devel("leaves: %ld [m=%d]\n",
			 node->nr_leaves_on_branch - 1, has_meta);

		/* Look further up the tree to see if we can collapse this node
		 * into a more proximal node too.
		 */
		parent = node;
	collapse_up:
		pr_devel("collapse subtree: %ld\n", parent->nr_leaves_on_branch);

		ptr = parent->back_pointer;
		if (!ptr)
			goto do_collapse;
		if (assoc_array_ptr_is_shortcut(ptr)) {
			struct assoc_array_shortcut *s = assoc_array_ptr_to_shortcut(ptr);
			ptr = s->back_pointer;
			if (!ptr)
				goto do_collapse;
		}

		grandparent = assoc_array_ptr_to_node(ptr);
		if (grandparent->nr_leaves_on_branch <= ASSOC_ARRAY_FAN_OUT + 1) {
			parent = grandparent;
			goto collapse_up;
		}

	do_collapse:
		/* There's no point collapsing if the original node has no meta
		 * pointers to discard and if we didn't merge into one of that
		 * node's ancestry.
		 */
		if (has_meta || parent != node) {
			node = parent;

			/* Create a new node to collapse into */
			new_n0 = kzalloc_obj(struct assoc_array_node);
			if (!new_n0)
				goto enomem;
			edit->new_meta[0] = assoc_array_node_to_ptr(new_n0);

			new_n0->back_pointer = node->back_pointer;
			new_n0->parent_slot = node->parent_slot;
			new_n0->nr_leaves_on_branch = node->nr_leaves_on_branch;
			edit->adjust_count_on = new_n0;

			collapse.node = new_n0;
			collapse.skip_leaf = assoc_array_ptr_to_leaf(edit->dead_leaf);
			collapse.slot = 0;
			assoc_array_subtree_iterate(assoc_array_node_to_ptr(node),
						    node->back_pointer,
						    assoc_array_delete_collapse_iterator,
						    &collapse);
			pr_devel("collapsed %d,%lu\n", collapse.slot, new_n0->nr_leaves_on_branch);
			BUG_ON(collapse.slot != new_n0->nr_leaves_on_branch - 1);

			if (!node->back_pointer) {
				edit->set[1].ptr = &array->root;
			} else if (assoc_array_ptr_is_leaf(node->back_pointer)) {
				BUG();
			} else if (assoc_array_ptr_is_node(node->back_pointer)) {
				struct assoc_array_node *p =
					assoc_array_ptr_to_node(node->back_pointer);
				edit->set[1].ptr = &p->slots[node->parent_slot];
			} else if (assoc_array_ptr_is_shortcut(node->back_pointer)) {
				struct assoc_array_shortcut *s =
					assoc_array_ptr_to_shortcut(node->back_pointer);
				edit->set[1].ptr = &s->next_node;
			}
			edit->set[1].to = assoc_array_node_to_ptr(new_n0);
			edit->excised_subtree = assoc_array_node_to_ptr(node);
		}
	}

	return edit;

enomem:
	/* Clean up after an out of memory error */
	pr_devel("enomem\n");
	assoc_array_cancel_edit(edit);
	return ERR_PTR(-ENOMEM);
}

/**
/* …（同文件无关代码省略）… */
static void assoc_array_rcu_cleanup(struct rcu_head *head)
{
	struct assoc_array_edit *edit =
		container_of(head, struct assoc_array_edit, rcu);
	int i;

	pr_devel("-->%s()\n", __func__);

	if (edit->dead_leaf)
		edit->ops->free_object(assoc_array_ptr_to_leaf(edit->dead_leaf));
	for (i = 0; i < ARRAY_SIZE(edit->excised_meta); i++)
		if (edit->excised_meta[i])
			kfree(assoc_array_ptr_to_node(edit->excised_meta[i]));

	if (edit->excised_subtree) {
		BUG_ON(assoc_array_ptr_is_leaf(edit->excised_subtree));
		if (assoc_array_ptr_is_node(edit->excised_subtree)) {
			struct assoc_array_node *n =
				assoc_array_ptr_to_node(edit->excised_subtree);
			n->back_pointer = NULL;
		} else {
			struct assoc_array_shortcut *s =
				assoc_array_ptr_to_shortcut(edit->excised_subtree);
			s->back_pointer = NULL;
		}
		assoc_array_destroy_subtree(edit->excised_subtree,
					    edit->ops_for_excised_subtree);
	}

	kfree(edit);
}
/* …（同文件无关代码省略）… */
void assoc_array_apply_edit(struct assoc_array_edit *edit)
{
	struct assoc_array_shortcut *shortcut;
	struct assoc_array_node *node;
	struct assoc_array_ptr *ptr;
	int i;

	pr_devel("-->%s()\n", __func__);

	smp_wmb();
	if (edit->leaf_p)
		*edit->leaf_p = edit->leaf;

	smp_wmb();
	for (i = 0; i < ARRAY_SIZE(edit->set_parent_slot); i++)
		if (edit->set_parent_slot[i].p)
			*edit->set_parent_slot[i].p = edit->set_parent_slot[i].to;

	smp_wmb();
	for (i = 0; i < ARRAY_SIZE(edit->set_backpointers); i++)
		if (edit->set_backpointers[i])
			*edit->set_backpointers[i] = edit->set_backpointers_to;

	smp_wmb();
	for (i = 0; i < ARRAY_SIZE(edit->set); i++)
		if (edit->set[i].ptr)
			*edit->set[i].ptr = edit->set[i].to;

	if (edit->array->root == NULL) {
		edit->array->nr_leaves_on_tree = 0;
	} else if (edit->adjust_count_on) {
		node = edit->adjust_count_on;
		for (;;) {
			node->nr_leaves_on_branch += edit->adjust_count_by;

			ptr = node->back_pointer;
			if (!ptr)
				break;
			if (assoc_array_ptr_is_shortcut(ptr)) {
				shortcut = assoc_array_ptr_to_shortcut(ptr);
				ptr = shortcut->back_pointer;
				if (!ptr)
					break;
			}
			BUG_ON(!assoc_array_ptr_is_node(ptr));
			node = assoc_array_ptr_to_node(ptr);
		}

		edit->array->nr_leaves_on_tree += edit->adjust_count_by;
	}

	call_rcu(&edit->rcu, assoc_array_rcu_cleanup);
}
/* …（同文件无关代码省略）… */
void assoc_array_cancel_edit(struct assoc_array_edit *edit)
{
	struct assoc_array_ptr *ptr;
	int i;

	pr_devel("-->%s()\n", __func__);

	/* Clean up after an out of memory error */
	for (i = 0; i < ARRAY_SIZE(edit->new_meta); i++) {
		ptr = edit->new_meta[i];
		if (ptr) {
			if (assoc_array_ptr_is_node(ptr))
				kfree(assoc_array_ptr_to_node(ptr));
			else
				kfree(assoc_array_ptr_to_shortcut(ptr));
		}
	}
	kfree(edit);
}

/**
 * assoc_array_gc - Garbage collect an associative array.
 * @array: The array to clean.
 * @ops: The operations to use.
 * @iterator: A callback function to pass judgement on each object.
 * @iterator_data: Private data for the callback function.
 *
 * Collect garbage from an associative array and pack down the internal tree to
 * save memory.
 *
 * The iterator function is asked to pass judgement upon each object in the
 * array.  If it returns false, the object is discard and if it returns true,
 * the object is kept.  If it returns true, it must increment the object's
 * usage count (or whatever it needs to do to retain it) before returning.
 *
 * This function returns 0 if successful or -ENOMEM if out of memory.  In the
 * latter case, the array is not changed.
 *
 * The caller should lock against other modifications and must continue to hold
 * the lock until assoc_array_apply_edit() has been called.
 *
 * Accesses to the tree may take place concurrently with this function,
 * provided they hold the RCU read lock.
 */
int assoc_array_gc(struct assoc_array *array,
		   const struct assoc_array_ops *ops,
		   bool (*iterator)(void *object, void *iterator_data),
		   void *iterator_data)
{
	struct assoc_array_shortcut *shortcut, *new_s;
	struct assoc_array_node *node, *new_n;
	struct assoc_array_edit *edit;
	struct assoc_array_ptr *cursor, *ptr;
	struct assoc_array_ptr *new_root, *new_parent, **new_ptr_pp;
	unsigned long nr_leaves_on_tree;
	bool retained;
	int keylen, slot, nr_free, next_slot, i;

	pr_devel("-->%s()\n", __func__);

	if (!array->root)
		return 0;

	edit = kzalloc_obj(struct assoc_array_edit);
	if (!edit)
		return -ENOMEM;
	edit->array = array;
	edit->ops = ops;
	edit->ops_for_excised_subtree = ops;
	edit->set[0].ptr = &array->root;
	edit->excised_subtree = array->root;

	new_root = new_parent = NULL;
	new_ptr_pp = &new_root;
	cursor = array->root;

descend:
	/* If this point is a shortcut, then we need to duplicate it and
	 * advance the target cursor.
	 */
	if (assoc_array_ptr_is_shortcut(cursor)) {
		shortcut = assoc_array_ptr_to_shortcut(cursor);
		keylen = round_up(shortcut->skip_to_level, ASSOC_ARRAY_KEY_CHUNK_SIZE);
		keylen >>= ASSOC_ARRAY_KEY_CHUNK_SHIFT;
		new_s = kmalloc_flex(*new_s, index_key, keylen);
		if (!new_s)
			goto enomem;
		pr_devel("dup shortcut %p -> %p\n", shortcut, new_s);
		memcpy(new_s, shortcut, struct_size(new_s, index_key, keylen));
		new_s->back_pointer = new_parent;
		new_s->parent_slot = shortcut->parent_slot;
		*new_ptr_pp = new_parent = assoc_array_shortcut_to_ptr(new_s);
		new_ptr_pp = &new_s->next_node;
		cursor = shortcut->next_node;
	}

	/* Duplicate the node at this position */
	node = assoc_array_ptr_to_node(cursor);
	new_n = kzalloc_obj(struct assoc_array_node);
	if (!new_n)
		goto enomem;
	pr_devel("dup node %p -> %p\n", node, new_n);
	new_n->back_pointer = new_parent;
	new_n->parent_slot = node->parent_slot;
	*new_ptr_pp = new_parent = assoc_array_node_to_ptr(new_n);
	new_ptr_pp = NULL;
	slot = 0;

continue_node:
	/* Filter across any leaves and gc any subtrees */
	for (; slot < ASSOC_ARRAY_FAN_OUT; slot++) {
		ptr = node->slots[slot];
		if (!ptr)
			continue;

		if (assoc_array_ptr_is_leaf(ptr)) {
			if (iterator(assoc_array_ptr_to_leaf(ptr),
				     iterator_data))
				/* The iterator will have done any reference
				 * counting on the object for us.
				 */
				new_n->slots[slot] = ptr;
			continue;
		}

		new_ptr_pp = &new_n->slots[slot];
		cursor = ptr;
		goto descend;
	}

retry_compress:
	pr_devel("-- compress node %p --\n", new_n);
/* …（同文件无关代码省略）… */
		goto retry_compress;
	}
	pr_devel("after: %lu\n", new_n->nr_leaves_on_branch);

	nr_leaves_on_tree = new_n->nr_leaves_on_branch;

	/* Excise this node if it is singly occupied by a shortcut */
	if (nr_free == ASSOC_ARRAY_FAN_OUT - 1) {
/* …（同文件无关代码省略）… */
			if (!new_parent) {
				new_s->back_pointer = NULL;
				new_s->parent_slot = 0;
				new_root = ptr;
				goto gc_complete;
			}

			if (assoc_array_ptr_is_shortcut(new_parent)) {
/* …（同文件无关代码省略）… */
				if (!new_parent) {
					new_s->back_pointer = NULL;
					new_s->parent_slot = 0;
					new_root = ptr;
					goto gc_complete;
				}
			}

			new_s->back_pointer = new_parent;
			new_s->parent_slot = slot;
			new_n = assoc_array_ptr_to_node(new_parent);
			new_n->slots[slot] = ptr;
			goto ascend_old_tree;
		}
	}

/* …（同文件无关代码省略）… */
	 */
	ptr = new_n->back_pointer;
	if (!ptr)
		goto gc_complete;

	if (assoc_array_ptr_is_shortcut(ptr)) {
		new_s = assoc_array_ptr_to_shortcut(ptr);
		new_parent = new_s->back_pointer;
		slot = new_s->parent_slot;

		if (new_n->nr_leaves_on_branch <= ASSOC_ARRAY_FAN_OUT) {
			struct assoc_array_node *n;

			pr_devel("excise shortcut\n");
			new_n->back_pointer = new_parent;
			new_n->parent_slot = slot;
			kfree(new_s);
			if (!new_parent) {
				new_root = assoc_array_node_to_ptr(new_n);
				goto gc_complete;
			}

			n = assoc_array_ptr_to_node(new_parent);
			n->slots[slot] = assoc_array_node_to_ptr(new_n);
		}
	} else {
		new_parent = ptr;
	}
	new_n = assoc_array_ptr_to_node(new_parent);

ascend_old_tree:
	ptr = node->back_pointer;
	if (assoc_array_ptr_is_shortcut(ptr)) {
		shortcut = assoc_array_ptr_to_shortcut(ptr);
		slot = shortcut->parent_slot;
		cursor = shortcut->back_pointer;
		if (!cursor)
			goto gc_complete;
	} else {
		slot = node->parent_slot;
		cursor = ptr;
	}
	BUG_ON(!cursor);
	node = assoc_array_ptr_to_node(cursor);
	slot++;
	goto continue_node;

gc_complete:
	edit->set[0].to = new_root;
	assoc_array_apply_edit(edit);
	array->nr_leaves_on_tree = nr_leaves_on_tree;
	return 0;

enomem:
	pr_devel("enomem\n");
	assoc_array_destroy_subtree(new_root, edit->ops);
	kfree(edit);
	return -ENOMEM;
}
