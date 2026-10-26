// AUTO-DRAFT from torvalds/linux PR #44dc57ed1e77a23248ef28732486f4b7e9468cf7
  // <<< BUG ANCHOR
static struct platform_device i2c_device = {
	.name		= "i2c-pca-platform",
	.id		= -1,
	.dev		= {
		.platform_data	= &i2c_platform_data,
	},
