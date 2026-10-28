// AUTO-DRAFT from torvalds/linux PR #857c32a3b82c4f4abb7addee394c4dbfc79ac6db
};
#endif /* MCFFLEXCAN_SIZE */

static struct platform_device *mcf_devices[] __initdata = {
	&mcf_uart,
#ifdef MCFFEC_BASE0
/* …（同文件无关代码省略）… */
#ifdef MCFFLEXCAN_SIZE
	&mcf_flexcan0,
#endif
};

/*
