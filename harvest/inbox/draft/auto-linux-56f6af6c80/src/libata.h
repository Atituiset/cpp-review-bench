// AUTO-DRAFT from torvalds/linux PR #fd179f8a05be3ccae366b9b96e176b51fbe54aab
	__ATA_QUIRK_MAX_SEC,		/* Limit max sectors */
	__ATA_QUIRK_MAX_TRIM_128M,	/* Limit max trim size to 128M */
	__ATA_QUIRK_NO_NCQ_ON_ATI,	/* Disable NCQ on ATI chipset */
	__ATA_QUIRK_NO_LPM_ON_ATI,	/* Disable LPM on ATI chipset */  // <<< BUG ANCHOR
	__ATA_QUIRK_NO_ID_DEV_LOG,	/* Identify device log missing */
	__ATA_QUIRK_NO_LOG_DIR,		/* Do not read log directory */
	__ATA_QUIRK_NO_FUA,		/* Do not use FUA */
/* …（同文件无关代码省略）… */
	ATA_QUIRK_MAX_SEC		= BIT_ULL(__ATA_QUIRK_MAX_SEC),
	ATA_QUIRK_MAX_TRIM_128M		= BIT_ULL(__ATA_QUIRK_MAX_TRIM_128M),
	ATA_QUIRK_NO_NCQ_ON_ATI		= BIT_ULL(__ATA_QUIRK_NO_NCQ_ON_ATI),
	ATA_QUIRK_NO_LPM_ON_ATI		= BIT_ULL(__ATA_QUIRK_NO_LPM_ON_ATI),
	ATA_QUIRK_NO_ID_DEV_LOG		= BIT_ULL(__ATA_QUIRK_NO_ID_DEV_LOG),
	ATA_QUIRK_NO_LOG_DIR		= BIT_ULL(__ATA_QUIRK_NO_LOG_DIR),
	ATA_QUIRK_NO_FUA		= BIT_ULL(__ATA_QUIRK_NO_FUA),
