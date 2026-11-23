// AUTO-DRAFT from torvalds/linux PR #6e4122f0242a3262512ee3e6f2e7e6de0bd5792b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>

#define X86_FEATURE_K8			( 3*32+ 4) /* Opteron, Athlon64 */
#define X86_FEATURE_ZEN5		( 3*32+ 5) /* CPU based on Zen5 microarchitecture */
#define X86_FEATURE_ZEN6		( 3*32+ 6) /* CPU based on Zen6 microarchitecture */
/* Free                                 ( 3*32+ 7) */
#define X86_FEATURE_CONSTANT_TSC	( 3*32+ 8) /* "constant_tsc" TSC ticks at a constant rate */
/* free: was #define X86_FEATURE_UP	( 3*32+ 9) * "up" SMP kernel running on UP */
#define X86_FEATURE_ART			( 3*32+10) /* "art" Always running timer (ART) */
/* …（同文件无关代码省略）… */
// free: was #define X86_FEATURE_PVUNLOCK               ( 8*32+20) /* PV unlock function */
#define X86_FEATURE_VCPUPREEMPT		( 8*32+21) /* PV vcpu_is_preempted function */
#define X86_FEATURE_TDX_GUEST		( 8*32+22) /* "tdx_guest" Intel Trust Domain Extensions Guest */

/* Intel-defined CPU features, CPUID level 0x00000007:0 (EBX), word 9 */
#define X86_FEATURE_FSGSBASE		( 9*32+ 0) /* "fsgsbase" RDFSBASE, WRFSBASE, RDGSBASE, WRGSBASE instructions*/
/* …（同文件无关代码省略）… */
#define X86_FEATURE_SUCCOR		(17*32+ 1) /* "succor" Uncorrectable error containment and recovery */
#define X86_FEATURE_CPPC_PERF_PRIO	(17*32+ 2) /* CPPC Floor Perf support */
#define X86_FEATURE_SMCA		(17*32+ 3) /* "smca" Scalable MCA */

/* Intel-defined CPU features, CPUID level 0x00000007:0 (EDX), word 18 */
#define X86_FEATURE_AVX512_4VNNIW	(18*32+ 2) /* "avx512_4vnniw" AVX-512 Neural Network Instructions */
/* …（同文件无关代码省略）… */
#define X86_FEATURE_AUTOIBRS		(20*32+ 8) /* Automatic IBRS */
#define X86_FEATURE_NO_SMM_CTL_MSR	(20*32+ 9) /* SMM_CTL MSR is not present */

#define X86_FEATURE_GP_ON_USER_CPUID	(20*32+17) /* User CPUID faulting */

#define X86_FEATURE_PREFETCHI		(20*32+20) /* Prefetch Data/Instruction to Cache Level */
