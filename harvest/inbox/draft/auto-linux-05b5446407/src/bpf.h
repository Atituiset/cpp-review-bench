// AUTO-DRAFT from torvalds/linux PR #a4da531b171c664a6cf42c5b82d1610cbdf14fe4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

struct bpf_iter_seq_info {
	const struct seq_operations *seq_ops;
	bpf_iter_init_seq_priv_t init_seq_private;
	bpf_iter_fini_seq_priv_t fini_seq_private;
	u32 seq_priv_size;
};
/* …（同文件无关代码省略）… */
struct bpf_map_ops {
	/* funcs callable from userspace (via syscall) */
	int (*map_alloc_check)(union bpf_attr *attr);
	struct bpf_map *(*map_alloc)(union bpf_attr *attr);
	void (*map_release)(struct bpf_map *map, struct file *map_file);
	void (*map_free)(struct bpf_map *map);
	void (*map_free_pre_rcu)(struct bpf_map *map);
	int (*map_get_next_key)(struct bpf_map *map, void *key, void *next_key);
	void (*map_release_uref)(struct bpf_map *map);
	void *(*map_lookup_elem_sys_only)(struct bpf_map *map, void *key);
	int (*map_lookup_batch)(struct bpf_map *map, const union bpf_attr *attr,
				union bpf_attr __user *uattr);
	int (*map_lookup_and_delete_elem)(struct bpf_map *map, void *key,
					  void *value, u64 flags);
	int (*map_lookup_and_delete_batch)(struct bpf_map *map,
					   const union bpf_attr *attr,
					   union bpf_attr __user *uattr);
	int (*map_update_batch)(struct bpf_map *map, struct file *map_file,
				const union bpf_attr *attr,
				union bpf_attr __user *uattr);
	int (*map_delete_batch)(struct bpf_map *map, const union bpf_attr *attr,
				union bpf_attr __user *uattr);

	/* funcs callable from userspace and from eBPF programs */
	void *(*map_lookup_elem)(struct bpf_map *map, void *key);
	long (*map_update_elem)(struct bpf_map *map, void *key, void *value, u64 flags);
	long (*map_delete_elem)(struct bpf_map *map, void *key);
	long (*map_push_elem)(struct bpf_map *map, void *value, u64 flags);
	long (*map_pop_elem)(struct bpf_map *map, void *value);
	long (*map_peek_elem)(struct bpf_map *map, void *value);
	void *(*map_lookup_percpu_elem)(struct bpf_map *map, void *key, u32 cpu);
	int (*map_get_hash)(struct bpf_map *map);

	/* funcs called by prog_array and perf_event_array map */
	void *(*map_fd_get_ptr)(struct bpf_map *map, struct file *map_file,
				int fd);
	/* If need_defer is true, the implementation should guarantee that
	 * the to-be-put element is still alive before the bpf program, which
	 * may manipulate it, exists.
	 */
	void (*map_fd_put_ptr)(struct bpf_map *map, void *ptr, bool need_defer);
	int (*map_gen_lookup)(struct bpf_map *map, struct bpf_insn *insn_buf);
	u32 (*map_fd_sys_lookup_elem)(void *ptr);
	void (*map_seq_show_elem)(struct bpf_map *map, void *key,
				  struct seq_file *m);
	int (*map_check_btf)(struct bpf_map *map,
			     const struct btf *btf,
			     const struct btf_type *key_type,
			     const struct btf_type *value_type);

	/* Prog poke tracking helpers. */
	int (*map_poke_track)(struct bpf_map *map, struct bpf_prog_aux *aux);
	void (*map_poke_untrack)(struct bpf_map *map, struct bpf_prog_aux *aux);
	void (*map_poke_run)(struct bpf_map *map, u32 key, struct bpf_prog *old,
			     struct bpf_prog *new);

	/* Direct value access helpers. */
	int (*map_direct_value_addr)(const struct bpf_map *map,
				     u64 *imm, u32 off);
	int (*map_direct_value_meta)(const struct bpf_map *map,
				     u64 imm, u32 *off);
	int (*map_mmap)(struct bpf_map *map, struct vm_area_struct *vma);
	vm_fault_t (*map_mmap_fault)(struct bpf_map *map, struct vm_fault *vmf);
	__poll_t (*map_poll)(struct bpf_map *map, struct file *filp,
			     struct poll_table_struct *pts);
	unsigned long (*map_get_unmapped_area)(struct file *filep, unsigned long addr,
					       unsigned long len, unsigned long pgoff,
					       unsigned long flags);

	/* Functions called by bpf_local_storage maps */
	int (*map_local_storage_charge)(struct bpf_local_storage_map *smap,
					void *owner, u32 size);
	void (*map_local_storage_uncharge)(struct bpf_local_storage_map *smap,
					   void *owner, u32 size);
	struct bpf_local_storage __rcu ** (*map_owner_storage_ptr)(void *owner);

	/* Misc helpers.*/
	long (*map_redirect)(struct bpf_map *map, u64 key, u64 flags);

	/* map_meta_equal must be implemented for maps that can be
	 * used as an inner map.  It is a runtime check to ensure
	 * an inner map can be inserted to an outer map.
	 *
	 * Some properties of the inner map has been used during the
	 * verification time.  When inserting an inner map at the runtime,
	 * map_meta_equal has to ensure the inserting map has the same
	 * properties that the verifier has used earlier.
	 */
	bool (*map_meta_equal)(const struct bpf_map *meta0,
			       const struct bpf_map *meta1);


	int (*map_set_for_each_callback_args)(struct bpf_verifier_env *env,
					      struct bpf_func_state *caller,
					      struct bpf_func_state *callee);
	long (*map_for_each_callback)(struct bpf_map *map,
				     bpf_callback_t callback_fn,
				     void *callback_ctx, u64 flags);

	u64 (*map_mem_usage)(const struct bpf_map *map);

	/* BTF id of struct allocated by map_alloc */
	int *map_btf_id;

	/* bpf_iter info used to open a seq_file */
	const struct bpf_iter_seq_info *iter_seq_info;
};
/* …（同文件无关代码省略）… */
enum btf_field_type {
	BPF_SPIN_LOCK  = (1 << 0),
	BPF_TIMER      = (1 << 1),
	BPF_KPTR_UNREF = (1 << 2),
	BPF_KPTR_REF   = (1 << 3),
	BPF_KPTR_PERCPU = (1 << 4),
	BPF_KPTR       = BPF_KPTR_UNREF | BPF_KPTR_REF | BPF_KPTR_PERCPU,
	BPF_LIST_HEAD  = (1 << 5),
	BPF_LIST_NODE  = (1 << 6),
	BPF_RB_ROOT    = (1 << 7),
	BPF_RB_NODE    = (1 << 8),
	BPF_GRAPH_NODE = BPF_RB_NODE | BPF_LIST_NODE,
	BPF_GRAPH_ROOT = BPF_RB_ROOT | BPF_LIST_HEAD,
	BPF_REFCOUNT   = (1 << 9),
	BPF_WORKQUEUE  = (1 << 10),
	BPF_UPTR       = (1 << 11),
	BPF_RES_SPIN_LOCK = (1 << 12),
	BPF_TASK_WORK  = (1 << 13),
	BPF_RCU_HEAD   = (1 << 14),
};
/* …（同文件无关代码省略）… */
#define MAX_BPF_CGROUP_STORAGE_TYPE __BPF_CGROUP_STORAGE_MAX
/* …（同文件无关代码省略）… */
struct btf_field_kptr {
	struct btf *btf;
	struct module *module;
	/* dtor used if btf_is_kernel(btf), otherwise the type is
	 * program-allocated, dtor is NULL,  and __bpf_obj_drop_impl is used
	 */
	btf_dto
