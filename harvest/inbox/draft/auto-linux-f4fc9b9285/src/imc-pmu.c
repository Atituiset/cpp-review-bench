// AUTO-DRAFT from torvalds/linux PR #a27611f8994c9a4772156432badb533af33fd32d
				    struct perf_event_header *header,
				    struct perf_event *event)
{
	/* Sanity checks for a valid record */
	if (be64_to_cpu(READ_ONCE(mem->tb1)) > *prev_tb)
		*prev_tb = be64_to_cpu(READ_ONCE(mem->tb1));
/* …（同文件无关代码省略）… */
	data->ip =  be64_to_cpu(READ_ONCE(mem->ip));
	data->period = event->hw.last_period;
  // <<< BUG ANCHOR
	header->type = PERF_RECORD_SAMPLE;
	header->size = sizeof(*header) + event->header_size;
	header->misc = 0;

	if (cpu_has_feature(CPU_FTR_ARCH_31)) {
		switch (IMC_TRACE_RECORD_VAL_HVPR(be64_to_cpu(READ_ONCE(mem->val)))) {
		case 0:/* when MSR HV and PR not set in the trace-record */
			header->misc |= PERF_RECORD_MISC_GUEST_KERNEL;
			break;
		case 1: /* MSR HV is 0 and PR is 1 */
			header->misc |= PERF_RECORD_MISC_GUEST_USER;
			break;
		case 2: /* MSR HV is 1 and PR is 0 */
			header->misc |= PERF_RECORD_MISC_KERNEL;
			break;
		case 3: /* MSR HV is 1 and PR is 1 */
			header->misc |= PERF_RECORD_MISC_USER;
			break;
		default:
			pr_info("IMC: Unable to set the flag based on MSR bits\n");
			break;
		}
	} else {
		if (is_kernel_addr(data->ip))
			header->misc |= PERF_RECORD_MISC_KERNEL;
		else
			header->misc |= PERF_RECORD_MISC_USER;
	}
	perf_event_header__init_id(header, data, event);

	return 0;
}
