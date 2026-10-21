// AUTO-DRAFT from torvalds/linux PR #0d508d4709837bcc88e314c0533a42d3c55b31e3
struct audit_aux_data {
	struct audit_aux_data	*next;
	int			type;
};
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
struct audit_aux_data_bprm_fcaps {
	struct audit_aux_data	d;
	struct audit_cap_data	fcap;
	unsigned int		fcap_ver;
	struct audit_cap_data	old_pcap;
	struct audit_cap_data	new_pcap;
/* …（同文件无关代码省略）… */
{
	struct audit_context *context = audit_context();

	context->type = AUDIT_EXECVE;
	context->execve.argc = bprm->argc;
}
/* …（同文件无关代码省略）… */
void __audit_log_nfcfg(const char *name, u8 af, unsigned int nentries,
		       enum audit_nfcfgop op, gfp_t gfp)
{
	struct audit_buffer *ab;
	char comm[sizeof(current->comm)];

	ab = audit_log_start(audit_context(), gfp, AUDIT_NETFILTER_CFG);
	if (!ab)
		return;
	audit_log_format(ab, "table=%s family=%u entries=%u op=%s",
			 name, af, nentries, audit_nfcfgs[op].s);

	audit_log_format(ab, " pid=%u", task_tgid_nr(current));
	audit_log_task_context(ab); /* subj= */
	audit_log_format(ab, " comm=");
	audit_log_untrustedstring(ab, get_task_comm(comm, current));
	audit_log_end(ab);
}
EXPORT_SYMBOL_GPL(__audit_log_nfcfg);

static void audit_log_task(struct audit_buffer *ab)
{
	kuid_t auid, uid;
