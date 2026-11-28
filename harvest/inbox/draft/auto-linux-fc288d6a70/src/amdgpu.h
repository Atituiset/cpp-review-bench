// AUTO-DRAFT from torvalds/linux PR #c639ac6b9cf100b860408a3cf8d7eab268866cd7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define AMDGPU_BIOS_NUM_SCRATCH			16
/* …（同文件无关代码省略）… */
#define AMDGPU_MAX_PPLL 3
/* …（同文件无关代码省略）… */
struct amdgpu_clock {
	struct amdgpu_pll ppll[AMDGPU_MAX_PPLL];
	struct amdgpu_pll spll;
	struct amdgpu_pll mpll;
	/* 10 Khz units */
	uint32_t default_mclk;
	uint32_t default_sclk;
	uint32_t default_dispclk;
	uint32_t dp_extclk;
	uint32_t max_pixel_clock;
};
/* …（同文件无关代码省略）… */
struct amdgpu_asic_funcs {
	bool (*read_disabled_bios)(struct amdgpu_device *adev);
	bool (*read_bios_from_rom)(struct amdgpu_device *adev,
				   u8 *bios, u32 length_bytes);
	int (*read_register)(struct amdgpu_device *adev, u32 se_num,
			     u32 sh_num, u32 reg_offset, u32 *value);
	void (*set_vga_state)(struct amdgpu_device *adev, bool state);
	int (*reset)(struct amdgpu_device *adev);
	enum amd_reset_method (*reset_method)(struct amdgpu_device *adev);
	/* get the reference clock */
	u32 (*get_xclk)(struct amdgpu_device *adev);
	/* MM block clocks */
	int (*set_uvd_clocks)(struct amdgpu_device *adev, u32 vclk, u32 dclk);
	int (*set_vce_clocks)(struct amdgpu_device *adev, u32 evclk, u32 ecclk);
	/* static power management */
	int (*get_pcie_lanes)(struct amdgpu_device *adev);
	void (*set_pcie_lanes)(struct amdgpu_device *adev, int lanes);
	/* get config memsize register */
	u32 (*get_config_memsize)(struct amdgpu_device *adev);
	/* flush hdp write queue */
	void (*flush_hdp)(struct amdgpu_device *adev, struct amdgpu_ring *ring);
	/* invalidate hdp read cache */
	void (*invalidate_hdp)(struct amdgpu_device *adev,
			       struct amdgpu_ring *ring);
	/* initialize doorbell layout for specific asic*/
	void (*init_doorbell_index)(struct amdgpu_device *adev);
	/* PCIe bandwidth usage */
	void (*get_pcie_usage)(struct amdgpu_device *adev, uint64_t *count0,
			       uint64_t *count1);
	/* do we need to reset the asic at init time (e.g., kexec) */
	bool (*need_reset_on_init)(struct amdgpu_device *adev);
	/* PCIe replay counter */
	uint64_t (*get_pcie_replay_count)(struct amdgpu_device *adev);
	/* device supports BACO */
	int (*supports_baco)(struct amdgpu_device *adev);
	/* pre asic_init quirks */
	void (*pre_asic_init)(struct amdgpu_device *adev);
	/* enter/exit umd stable pstate */
	int (*update_umd_stable_pstate)(struct amdgpu_device *adev, bool enter);
	/* query video codecs */
	int (*query_video_codecs)(struct amdgpu_device *adev, bool encode,
				  const struct amdgpu_video_codecs **codecs);
	/* encode "> 32bits" smn addressing */
	u64 (*encode_ext_smn_addressing)(int ext_id);

	ssize_t (*get_reg_state)(struct amdgpu_device *adev,
				 enum amdgpu_reg_state reg_state, void *buf,
				 size_t max_size);
	/* query FW reserved region (size/offset) from discovery */
	void (*get_fw_reserved_info)(struct amdgpu_device *adev,
				     u64 *reserve_size, u64 *offset);
};
/* …（同文件无关代码省略）… */
struct amdgpu_mem_scratch {
	struct amdgpu_bo		*robj;
	uint32_t			*ptr;
	u64				gpu_addr;
};
/* …（同文件无关代码省略）… */
struct amdgpu_mmio_remap {
	u32 reg_offset;
	resource_size_t bus_addr;
	struct amdgpu_bo *bo;
};
/* …（同文件无关代码省略）… */
#define AMDGPU_UID_INST_MAX 8 /* max number of instances for each UID type */
/* …（同文件无关代码省略）… */
struct amdgpu_uid {
	uint64_t uid[AMDGPU_UID_TYPE_MAX][AMDGPU_UID_INST_MAX];
	struct amdgpu_device *adev;
};
/* …（同文件无关代码省略）… */
struct amd_powerplay {
	void *pp_handle;
	const struct amd_pm_funcs *pp_funcs;
};
/* …（同文件无关代码省略）… */
struct amdgpu_pcie_reset_ctx {
	bool in_link_reset;
	bool occurs_dpc;
	bool audio_suspended;
	struct pci_dev *swus;
	struct pci_saved_state *swus_pcistate;
	struct pci_saved_state *swds_pcistate;
};
/* …（同文件无关代码省略）… */
enum amdgpu_init_lvl_id {
	AMDGPU_INIT_LEVEL_DEFAULT,
	AMDGPU_INIT_LEVEL_MINIMAL_XGMI,
	AMDGPU_INIT_LEVEL_RESET_RECOVERY,
};
/* …（同文件无关代码省略）… */
struct amdgpu_init_level {
	enum amdgpu_init_lvl_id level;
	uint32_t hwini_ip_block_mask;
};
/* …（同文件无关代码省略）… */
#define AMDGPU_RESET_MAGIC_NUM 64
#define AMDGPU_MAX_DF_PERFMONS 4
/* …（同文件无关代码省略）… */
enum amdgpu_enforce_isolation_mode {
	AMDGPU_ENFORCE_ISOLATION_DISABLE = 0,
	AMDGPU_ENFORCE_ISOLATION_ENABLE = 1,
	AMDGPU_ENFORCE_ISOLATION_ENABLE_LEGACY = 2,
	AMDGPU_ENFORCE_ISOLATION_NO_CLEANER_SHADER = 3,
};
/* …（同文件无关代码省略）… */
struct amdgpu_device {
	struct device			*dev;
	struct pci_dev			*pdev;
	/* The two ends of the physical PCIe link outside the device. */
	struct pci_dev			*link_dev;
	struct pci_dev			*link_partner;
	struct drm_device		ddev;

#ifdef CONFIG_DRM_AMD_ACP
	struct amdgpu_acp		acp;
#endif
	struct amdgpu_hive_info *hive;
	struct amdgpu_xcp_mgr *xcp_mgr;
	/* ASIC */
	enum amd_asic_type		asic_type;
	uint32_t			family;
	uint32_t			rev_id;
	uint32_t			external_rev_id;
	unsigned long			flags;
	unsigned long			apu_flags;
	int				usec_timeout;
	const struct amdgpu_asic_funcs	*asic_funcs;
	bool				shutdown;
	bool				need_swiotlb;
	bool				accel_working;
	struct notifier_block		acpi_nb;
	struct notifier_block		pm_nb;
	struct amdgpu_i2c_chan		*i2c_bus[AMDGPU_MAX_I2C_BUS];
	struct debugfs_blob_wrapper debugfs_vbios_blob;
	struct mutex			srbm_mutex;
	/* GRBM index mutex. Protects concurrent access to GRBM index */
	struct mutex                    grbm_idx_mutex;
	struct dev_pm_domain		vga_pm_domain;
	bool				have_disp_power_ref;
	bool                            have_atomics_support;
	bool				is_sw_smu;

	/* BIOS */
	bool				is_atom_fw;
	uint8_t				*bios;
	uint32_t			bios_size;
	uint32_t			bios_scratch_reg_offset;
	uint32_t			bios_scratch[AMDGPU_BIOS_NUM_SCRATCH];

	/* Register/doorbell mmio */
	resource_size_t			rmmio_base;
	resource_size_t			rmmio_size;
	void __iomem			*rmmio;
	/* protects concurrent MM_INDEX/DATA based register access */
	spinlock_t mmio_idx_lock;
	struct amdgpu_mmio_remap        rmmio_remap;
	/* Indirect register access blocks */
	struct amdgpu_reg_access reg;
	struct amdgpu_doorbell		doorbell;

	/* clock/pll info */
	struct amdgpu_clock            clock;

	/* MC */
	struc
