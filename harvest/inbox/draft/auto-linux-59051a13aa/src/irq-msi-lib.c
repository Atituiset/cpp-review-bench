// AUTO-DRAFT from torvalds/linux PR #65cac1217004da03c3d2418bd7a7fda038117809
		info->flags = MSI_FLAG_ALLOC_SIMPLE_MSI_DESCS | MSI_FLAG_FREE_MSI_DESCS;
		fallthrough;
	case DOMAIN_BUS_WIRED_TO_MSI:
		/* Remove PCI specific flags */
		required_flags &= ~MSI_FLAG_PCI_MSI_MASK_PARENT;
		break;
