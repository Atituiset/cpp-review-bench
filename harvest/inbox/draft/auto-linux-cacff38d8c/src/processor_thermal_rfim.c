// AUTO-DRAFT from torvalds/linux PR #d3e1fcc93027bec7c470bc4dfae4716e621f5cde
			dlvr_mmio_regs_table = lnl_dlvr_mmio_regs;
			dlvr_mapping = lnl_dlvr_mapping;
			break;
		case PCI_DEVICE_ID_INTEL_NVL_H_THERMAL:  // <<< BUG ANCHOR
		case PCI_DEVICE_ID_INTEL_NVL_S_THERMAL:
			dlvr_mmio_regs_table = nvl_dlvr_mmio_regs;
			ddr_data_rate_reg = &nvl_ddr_data_rate_reg;
			break;
