// AUTO-DRAFT from torvalds/linux PR #6addb4f385570ebc11c4eb499a4f1c149f313e84
#define REG_MAX				152
#define ADEC_MAX			152
#define NUM_CONTROLLERS			8
/* …（同文件无关代码省略）… */
union ecc_error_info {
	struct {
		u32 burstpos:3;
		u32 lrank:4;
		u32 rank:2;
		u32 group:3;
		u32 bank:2;
		u32 col:11;
		u32 row:7;
		u32 rowhi;
	};
	u64 i;
} __packed;
/* …（同文件无关代码省略）… */
struct ecc_status {
	union ecc_error_info ceinfo[2];
	union ecc_error_info ueinfo[2];
	u8 channel;
	u8 error_type;
};
/* …（同文件无关代码省略）… */
struct mc_priv {
	char message[256];
	struct ecc_status stat;
	u32 error_id;
	u32 error_level;
	u32 dwidth;
	u32 part_len;
	u32 regs[REG_MAX];
	u32 adec[ADEC_MAX];
	struct mem_ctl_info *mci[NUM_CONTROLLERS];
	struct rpmsg_endpoint *ept;
	struct cdx_mcdi *mcdi;
};
/* …（同文件无关代码省略）… */
static void remove_one_mc(struct mc_priv *priv, int i)
{
	struct mem_ctl_info *mci;

	mci = priv->mci[i];
	device_unregister(mci->pdev);
	edac_mc_del_mc(mci->pdev);
	edac_mc_free(mci);
}
/* …（同文件无关代码省略）… */
static void remove_versalnet(struct mc_priv *priv)
{
	for (int i = 0; i < NUM_CONTROLLERS; i++)
		remove_one_mc(priv, i);
}
/* …（同文件无关代码省略）… */
	remove_versalnet(priv);
	rproc_shutdown(priv->mcdi->r5_rproc);
	cdx_mcdi_finish(priv->mcdi);
	kfree(priv->mcdi);
}
