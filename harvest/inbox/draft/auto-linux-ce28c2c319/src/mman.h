// AUTO-DRAFT from torvalds/linux PR #65ba8f34ec886ec72b2c07ddc8cf3d815c32869d
#define MADV_HUGEPAGE	14		/* Worth backing with hugepages */
#define MADV_NOHUGEPAGE	15		/* Not worth backing with hugepages */

#define MADV_DONTDUMP   16		/* Explicity exclude from the core dump,
					   overrides the coredump filter bits */
#define MADV_DODUMP	17		/* Clear the MADV_NODUMP flag */
