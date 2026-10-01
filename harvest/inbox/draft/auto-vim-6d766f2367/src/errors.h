// AUTO-DRAFT from vim/vim PR #13354
EXTERN char e_xattr_e2big[]
	INIT(= N_("E1508: Size of the extended attribute value is larger than the maximum size allowed"));
EXTERN char e_xattr_other[]
	INIT(= N_("E1509: Error occured when reading or writing extended attribute"));  // <<< BUG ANCHOR
