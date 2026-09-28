// AUTO-DRAFT from torvalds/linux PR #efb44d93a620c294c049db37c810c8ab7ee1122d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static int svsm_perform_ghcb_protocol(struct ghcb *ghcb, struct svsm_call *call)
{
	struct es_em_ctxt ctxt;
	u8 pending = 0;

	vc_ghcb_invalidate(ghcb);

	/*
	 * Fill in protocol and format specifiers. This can be called very early
	 * in the boot, so use rip-relative references as needed.
	 */
	ghcb->protocol_version = ghcb_version;
	ghcb->ghcb_usage       = GHCB_DEFAULT_USAGE;

	ghcb_set_sw_exit_code(ghcb, SVM_VMGEXIT_SNP_RUN_VMPL);
	ghcb_set_sw_exit_info_1(ghcb, 0);
	ghcb_set_sw_exit_info_2(ghcb, 0);

	sev_es_wr_ghcb_msr(__pa(ghcb));

	svsm_issue_call(call, &pending);

	if (pending)
		return -EINVAL;

	switch (verify_exception_info(ghcb, &ctxt)) {
	case ES_OK:
		break;
	case ES_EXCEPTION:
		vc_forward_exception(&ctxt);
		fallthrough;
	default:
		return -EINVAL;
	}

	return svsm_process_result_codes(call);
}
/* …（同文件无关代码省略）… */
int svsm_perform_call_protocol(struct svsm_call *call)
{
	struct ghcb_state state;
	unsigned long flags;
	struct ghcb *ghcb;
	int ret;

	flags = native_local_irq_save();

	ghcb = __sev_get_ghcb(&state);

	do {
		ret = ghcb ? svsm_perform_ghcb_protocol(ghcb, call)
			   : __pi_svsm_perform_msr_protocol(call);
	} while (ret == -EAGAIN);

	__sev_put_ghcb(&state);

	native_local_irq_restore(flags);

	return ret;
}
/* …（同文件无关代码省略）… */
{
	struct svsm_call call = {};

	call.caa = svsm_get_caa();
	call.rax = SVSM_VTPM_CALL(SVSM_VTPM_CMD);
	call.rcx = __pa(buffer);

/* …（同文件无关代码省略）… */
	if (!snp_vmpl)
		return false;

	call.caa = svsm_get_caa();
	call.rax = SVSM_VTPM_CALL(SVSM_VTPM_QUERY);

	if (svsm_perform_call_protocol(&call))
