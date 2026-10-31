// AUTO-DRAFT from torvalds/linux PR #dd384658c738d15783a786d3af2851e02490f171
#define NCAPINTS			22	   /* N 32-bit words worth of info */
/* …（同文件无关代码省略）… */
#define X86_FEATURE_HYPERVISOR		( 4*32+31) /* "hypervisor" Running on a hypervisor */

/* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001, word 5 */
#define X86_FEATURE_XSTORE		( 5*32+ 2) /* "rng" RNG present (xstore) */
#define X86_FEATURE_XSTORE_EN		( 5*32+ 3) /* "rng_en" RNG enabled */
#define X86_FEATURE_XCRYPT		( 5*32+ 6) /* "ace" on-CPU crypto (xcrypt) */
#define X86_FEATURE_XCRYPT_EN		( 5*32+ 7) /* "ace_en" on-CPU crypto enabled */
#define X86_FEATURE_ACE2		( 5*32+ 8) /* "ace2" Advanced Cryptography Engine v2 */
/* …（同文件无关代码省略）… */
#define X86_FEATURE_PHE_EN		( 5*32+11) /* "phe_en" PHE enabled */
#define X86_FEATURE_PMM			( 5*32+12) /* "pmm" PadLock Montgomery Multiplier */
#define X86_FEATURE_PMM_EN		( 5*32+13) /* "pmm_en" PMM enabled */

/* More extended AMD flags: CPUID level 0x80000001, ECX, word 6 */
#define X86_FEATURE_LAHF_LM		( 6*32+ 0) /* "lahf_lm" LAHF/SAHF in long mode */
/* …（同文件无关代码省略）… */
#define X86_FEATURE_HWP_HIGHEST_PERF_CHANGE (14*32+15) /* HWP Highest perf change */
#define X86_FEATURE_HFI			(14*32+19) /* "hfi" Hardware Feedback Interface */

/* AMD SVM Feature Identification, CPUID level 0x8000000a (EDX), word 15 */
#define X86_FEATURE_NPT			(15*32+ 0) /* "npt" Nested Page Table support */
#define X86_FEATURE_LBRV		(15*32+ 1) /* "lbrv" LBR Virtualization support */
/* …（同文件无关代码省略）… */
#define X86_FEATURE_NO_NESTED_DATA_BP	(20*32+ 0) /* No Nested Data Breakpoints */
#define X86_FEATURE_WRMSR_XX_BASE_NS	(20*32+ 1) /* WRMSR to {FS,GS,KERNEL_GS}_BASE is non-serializing */
#define X86_FEATURE_LFENCE_RDTSC	(20*32+ 2) /* LFENCE always serializing / synchronizes RDTSC */
#define X86_FEATURE_VERW_CLEAR		(20*32+ 5) /* The memory form of VERW mitigates TSA */
#define X86_FEATURE_NULL_SEL_CLR_BASE	(20*32+ 6) /* Null Selector Clears Base */

/* …（同文件无关代码省略）… */
#define X86_BUG(x)			(NCAPINTS*32 + (x))
/* …（同文件无关代码省略）… */
#define X86_BUG_TSA			X86_BUG( 1*32+ 9) /* "tsa" CPU is affected by Transient Scheduler Attacks */
#define X86_BUG_VMSCAPE			X86_BUG( 1*32+10) /* "vmscape" CPU is affected by VMSCAPE attacks from guests */
#define X86_BUG_SEAMRET_INVD_VMCS	X86_BUG( 1*32+11) /* "seamret_invd_vmcs" SEAMRET from P-SEAMLDR clears the current VMCS */
#endif /* _ASM_X86_CPUFEATURES_H */
