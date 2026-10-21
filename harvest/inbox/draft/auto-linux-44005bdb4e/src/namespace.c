// AUTO-DRAFT from torvalds/linux PR #a804c5742bff224d4cb4d25dd383554d598be32b
static void dec_mnt_namespaces(struct ucounts *ucounts)
{
	dec_ucount(ucounts, UCOUNT_MNT_NAMESPACES);
}
  // <<< BUG ANCHOR
static void free_mnt_ns(struct mnt_namespace *ns)
{
	if (!is_anon_ns(ns))
		ns_common_free(ns);
	dec_mnt_namespaces(ns->ucounts);
	/* the last active reference is gone, no mark can show up anymore */
	fsnotify_mntns_delete(ns);
