// AUTO-DRAFT from torvalds/linux PR #f47d223627f80cb7a1eb14c585fe6cb35de5e984
	return calls->spu_run(fd_file(arg), unpc, ustatus);
}
  // <<< BUG ANCHOR
#ifdef CONFIG_COREDUMP
int elf_coredump_extra_notes_size(void)
{
	CLASS(spufs_calls, calls)();
	if (!calls)
		return 0;

	return calls->coredump_extra_notes_size();
}

int elf_coredump_extra_notes_write(struct coredump_params *cprm)
{
	CLASS(spufs_calls, calls)();
	if (!calls)
		return 0;

	return calls->coredump_extra_notes_write(cprm);
}
#endif

void notify_spus_active(void)
{
	struct spufs_calls *calls;
