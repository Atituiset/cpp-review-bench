// AUTO-DRAFT from torvalds/linux PR #59a02493f3b68fd0ad68c661393f680bcca5a85b
#define C(a, b)	a
/* …（同文件无关代码省略）… */
enum filter_pred_fn {
	FILTER_PRED_FN_NOP,
	FILTER_PRED_FN_64,
	FILTER_PRED_FN_64_CPUMASK,
	FILTER_PRED_FN_S64,
	FILTER_PRED_FN_U64,
	FILTER_PRED_FN_32,
	FILTER_PRED_FN_32_CPUMASK,
	FILTER_PRED_FN_S32,
	FILTER_PRED_FN_U32,
	FILTER_PRED_FN_16,
	FILTER_PRED_FN_16_CPUMASK,
	FILTER_PRED_FN_S16,
	FILTER_PRED_FN_U16,
	FILTER_PRED_FN_8,
	FILTER_PRED_FN_8_CPUMASK,
	FILTER_PRED_FN_S8,
	FILTER_PRED_FN_U8,
	FILTER_PRED_FN_COMM,
	FILTER_PRED_FN_STRING,
	FILTER_PRED_FN_STRLOC,
	FILTER_PRED_FN_STRRELLOC,
	FILTER_PRED_FN_PCHAR_USER,
	FILTER_PRED_FN_PCHAR,
	FILTER_PRED_FN_CPU,
	FILTER_PRED_FN_CPU_CPUMASK,
	FILTER_PRED_FN_CPUMASK,
	FILTER_PRED_FN_CPUMASK_CPU,
	FILTER_PRED_FN_FUNCTION,
	FILTER_PRED_FN_,
	FILTER_PRED_TEST_VISITED,
};
/* …（同文件无关代码省略）… */
struct filter_pred {
	struct regex		*regex;
	struct cpumask          *mask;
	unsigned short		*ops;
	struct ftrace_event_field *field;
	u64			val;
	u64			val2;
	enum filter_pred_fn	fn_num;
	int			offset;
	int			not;
	int			op;
};
/* …（同文件无关代码省略）… */
	C(IP_FIELD_ONLY,	"Only 'ip' field is supported for function trace"), \
	C(INVALID_VALUE,	"Invalid value (did you forget quotes)?"), \
	C(NO_FUNCTION,		"Function not found"),			\
	C(ERRNO,		"Error"),				\
	C(NO_FILTER,		"No filter found")
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
			top--;					/* #12 */
		}
	}
 out:
	if (top != op_stack) {
		/* Too many '(' */
/* …（同文件无关代码省略）… */
static inline int
do_filter_cpumask_scalar(int op, const struct cpumask *mask, unsigned int cpu)
{
	switch (op) {
	case OP_EQ:
		return cpumask_test_cpu(cpu, mask) &&
			cpumask_nth(1, mask) >= nr_cpu_ids;
	case OP_NE:
		return !cpumask_test_cpu(cpu, mask) ||
			cpumask_nth(1, mask) < nr_cpu_ids;
	case OP_BAND:
		return cpumask_test_cpu(cpu, mask);
	default:
		return 0;
	}
}
/* …（同文件无关代码省略）… */
static int filter_pred_cpumask_cpu(struct filter_pred *pred, void *event)
{
	u32 item = *(u32 *)(event + pred->offset);
	int loc = item & 0xffff;
	const struct cpumask *mask = (event + loc);
	unsigned int cpu = pred->val;

	return do_filter_cpumask_scalar(pred->op, mask, cpu);
}
/* …（同文件无关代码省略）… */
static int filter_pred_function(struct filter_pred *pred, void *event)
{
	unsigned long *addr = (unsigned long *)(event + pred->offset);
	unsigned long start = (unsigned long)pred->val;
	unsigned long end = (unsigned long)pred->val2;
	int ret = *addr >= start && *addr < end;

	return pred->op == OP_EQ ? ret : !ret;
}

/*
 * regex_match_foo - Basic regex callbacks
 *
/* …（同文件无关代码省略）… */
static int test_pred_visited_fn(struct filter_pred *pred, void *event)
{
	return 0;
}
/* …（同文件无关代码省略）… */
		return filter_pred_cpumask_cpu(pred, event);
	case FILTER_PRED_FN_FUNCTION:
		return filter_pred_function(pred, event);
	case FILTER_PRED_TEST_VISITED:
		return test_pred_visited_fn(pred, event);
	default:
/* …（同文件无关代码省略）… */

		} else if (field->filter_type == FILTER_DYN_STRING) {
			pred->fn_num = FILTER_PRED_FN_STRLOC;
		} else if (field->filter_type == FILTER_RDYN_STRING)
			pred->fn_num = FILTER_PRED_FN_STRRELLOC;
		else {

			if (!ustring_per_cpu) {
				/* Once allocated, keep it around for good */
				ustring_per_cpu = alloc_percpu(struct ustring_buffer);
