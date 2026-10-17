// AUTO-DRAFT from torvalds/linux PR #58161ead29e50d70a317ea26b472e0c7508e69da
struct rocket_device {
	struct drm_device ddev;

	struct mutex sched_lock;

	struct rocket_core *cores;
	unsigned int num_cores;
};

struct rocket_device *rocket_device_init(struct platform_device *pdev,
