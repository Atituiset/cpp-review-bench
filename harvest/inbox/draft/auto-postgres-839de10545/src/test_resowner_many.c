// AUTO-DRAFT from postgres/postgres PR #5f36942fd3196259ad5ad691f5ecbe4ce555ab7c
typedef struct
{
	ResourceOwnerDesc desc;
	int			nremembered;
	int			nforgotten;
	int			nreleased;
	int			nleaked;
  // <<< BUG ANCHOR
	dlist_head	current_resources;
} ManyTestResourceKind;
/* …（同文件无关代码省略）… */
	ManyTestResourceKind *after_kinds;

	/* Sanity check the arguments */
	if (nkinds < 0)
		elog(ERROR, "nkinds must be >= 0");
	if (nremember_bl < 0)
		elog(ERROR, "nremember_bl must be >= 0");
	if (nforget_bl < 0 || nforget_bl > nremember_bl)
		elog(ERROR, "nforget_bl must between 0 and 'nremember_bl'");
	if (nremember_al < 0)
		elog(ERROR, "nremember_al must be greater than zero");
	if (nforget_al < 0 || nforget_al > nremember_al)
		elog(ERROR, "nforget_al must between 0 and 'nremember_al'");
