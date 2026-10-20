// AUTO-DRAFT from torvalds/linux PR #f47d223627f80cb7a1eb14c585fe6cb35de5e984
	(0x7ff >> (PAGE_SHIFT - 12)) : \
	(0x3ffff >> (PAGE_SHIFT - 12)))

#ifdef CONFIG_SPU_BASE
/* Notes used in ET_CORE. Note name is "SPU/<fd>/<filename>". */
#define NT_SPU		1

#endif /* CONFIG_SPU_BASE */

#ifdef CONFIG_PPC64

#define get_cache_geometry(level) \
