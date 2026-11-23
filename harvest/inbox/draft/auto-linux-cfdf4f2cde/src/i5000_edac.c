// AUTO-DRAFT from torvalds/linux PR #1fadec005439186c67ca7be35c4e90727bc38fae
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define NUM_MTRS		4
#define CHANNELS_PER_BRANCH	2
/* …（同文件无关代码省略）… */
struct i5000_dimm_info {
	int megabytes;		/* size, 0 means not present  */
	int dual_rank;
};
/* …（同文件无关代码省略）… */
#define	MAX_CHANNELS	6	/* max possible channels */
#define MAX_CSROWS	(8*2)	/* max possible csrows per channel */
/* …（同文件无关代码省略）… */
struct i5000_pvt {
	struct pci_dev *system_address;	/* 16.0 */
	struct pci_dev *branchmap_werrors;	/* 16.1 */
	struct pci_dev *fsb_error_regs;	/* 16.2 */
	struct pci_dev *branch_0;	/* 21.0 */
	struct pci_dev *branch_1;	/* 22.0 */

	u16 tolm;		/* top of low memory */
	union {
		u64 ambase;		/* AMB BAR */
		struct {
			u32 ambase_bottom;
			u32 ambase_top;
		} u __packed;
	};

	u16 mir0, mir1, mir2;

	u16 b0_mtr[NUM_MTRS];	/* Memory Technology Reg */
	u16 b0_ambpresent0;	/* Branch 0, Channel 0 */
	u16 b0_ambpresent1;	/* Branch 0, Channel 1 */

	u16 b1_mtr[NUM_MTRS];	/* Memory Technology Reg */
	u16 b1_ambpresent0;	/* Branch 1, Channel 8 */
	u16 b1_ambpresent1;	/* Branch 1, Channel 1 */

	/* DIMM information matrix, allocating architecture maximums */
	struct i5000_dimm_info dimm_info[MAX_CSROWS][MAX_CHANNELS];

	/* Actual values for this controller */
	int maxch;		/* Max channels */
	int maxdimmperch;	/* Max DIMMs per channel */

	/* Hardware error reporting status */
	bool enabled_error_reporting;
};
/* …（同文件无关代码省略）… */
static int determine_mtr(struct i5000_pvt *pvt, int slot, int channel)
{
	int mtr;

	if (channel < CHANNELS_PER_BRANCH)
		mtr = pvt->b0_mtr[slot];
	else
		mtr = pvt->b1_mtr[slot];

	return mtr;
}
/* …（同文件无关代码省略）… */
	struct i5000_pvt *pvt;
	struct dimm_info *dimm;
	int empty;
	int max_csrows;
	int mtr;
	int csrow_megs;
	int channel;
	int slot;

	pvt = mci->pvt_info;
	max_csrows = pvt->maxdimmperch * 2;

	empty = 1;		/* Assume NO memory */

/* …（同文件无关代码省略）… */
	 * to map the dimms. A good cleanup would be to remove this array,
	 * and do a loop here with branch, channel, slot
	 */
	for (slot = 0; slot < max_csrows; slot++) {
		for (channel = 0; channel < pvt->maxch; channel++) {

			mtr = determine_mtr(pvt, slot, channel);
