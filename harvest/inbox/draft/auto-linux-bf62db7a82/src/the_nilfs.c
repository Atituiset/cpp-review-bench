// AUTO-DRAFT from torvalds/linux PR #d28fb91dd82aa56359b6de4d6241b7063672a4c0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	if (!new)
		return NULL;

	spin_lock(&nilfs->ns_cptree_lock);

	p = &nilfs->ns_cptree.rb_node;
/* …（同文件无关代码省略）… */
		} else {
			refcount_inc(&root->count);
			spin_unlock(&nilfs->ns_cptree_lock);
			kfree(new);
			return root;
		}
	}

	new->cno = cno;
	new->ifile = NULL;
	new->nilfs = nilfs;
	refcount_set(&new->count, 1);
	atomic64_set(&new->inodes_count, 0);
	atomic64_set(&new->blocks_count, 0);

	rb_link_node(&new->rb_node, parent, p);
	rb_insert_color(&new->rb_node, &nilfs->ns_cptree);

	spin_unlock(&nilfs->ns_cptree_lock);

	err = nilfs_sysfs_create_snapshot_group(new);
	if (err) {
		kfree(new);
		new = NULL;
	}

	return new;
}
