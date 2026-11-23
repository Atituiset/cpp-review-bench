// AUTO-DRAFT from torvalds/linux PR #6e4122f0242a3262512ee3e6f2e7e6de0bd5792b
#define SVM_EXIT_AVIC_INCOMPLETE_IPI		0x401
#define SVM_EXIT_AVIC_UNACCELERATED_ACCESS	0x402
#define SVM_EXIT_VMGEXIT       0x403

/* SEV-ES software-defined VMGEXIT events */
#define SVM_VMGEXIT_MMIO_READ			0x80000001ull
#define SVM_VMGEXIT_MMIO_WRITE			0x80000002ull
#define SVM_VMGEXIT_NMI_COMPLETE		0x80000003ull
/* …（同文件无关代码省略）… */
	{ SVM_EXIT_AVIC_INCOMPLETE_IPI,		"avic_incomplete_ipi" }, \
	{ SVM_EXIT_AVIC_UNACCELERATED_ACCESS,   "avic_unaccelerated_access" }, \
	{ SVM_EXIT_VMGEXIT,		"vmgexit" }, \
	{ SVM_VMGEXIT_MMIO_READ,	"vmgexit_mmio_read" }, \
	{ SVM_VMGEXIT_MMIO_WRITE,	"vmgexit_mmio_write" }, \
	{ SVM_VMGEXIT_NMI_COMPLETE,	"vmgexit_nmi_complete" }, \
