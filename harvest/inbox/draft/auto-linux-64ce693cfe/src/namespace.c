// AUTO-DRAFT from torvalds/linux PR #2cca8a6f6ae690755862f5b633125119dcf6bd0c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static struct mount *next_mnt(struct mount *p, struct mount *root)
{
	struct list_head *next = p->mnt_mounts.next;
	if (next == &p->mnt_mounts) {
		while (1) {
			if (p == root)
				return NULL;
			next = p->mnt_child.next;
			if (next != &p->mnt_parent->mnt_mounts)
				break;
			p = p->mnt_parent;
		}
	}
	return list_entry(next, struct mount, mnt_child);
}
/* …（同文件无关代码省略）… */
static struct mount *skip_mnt_tree(struct mount *p)
{
	struct list_head *prev = p->mnt_mounts.prev;
	while (prev != &p->mnt_mounts) {
		p = list_entry(prev, struct mount, mnt_child);
		prev = p->mnt_mounts.prev;
	}
	return p;
}
/* …（同文件无关代码省略）… */
	}

	new_ns->root = new_ns_root;
	ns_tree_add_raw(new_ns);
	return new_ns;
}

/* …（同文件无关代码省略）… */
static void dec_mnt_namespaces(struct ucounts *ucounts)
{
	dec_ucount(ucounts, UCOUNT_MNT_NAMESPACES);
}
/* …（同文件无关代码省略）… */
		dec_mnt_namespaces(ucounts);
		return ERR_PTR(ret);
	}
	ns_tree_gen_id(new_ns);

	new_ns->is_anon = anon;
	refcount_set(&new_ns->passive, 1);
	new_ns->mounts = RB_ROOT;
/* …（同文件无关代码省略）… */
				p = next_mnt(skip_mnt_tree(p), old);
		}
	}
	ns_tree_add_raw(new_ns);
	return new_ns;
}
