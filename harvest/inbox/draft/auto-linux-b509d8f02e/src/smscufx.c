// AUTO-DRAFT from torvalds/linux PR #0dfc0090b925243490b5b71bc9a229517cda499a
if (fbdefio) {
			fbdefio->delay = UFX_DEFIO_WRITE_DELAY;
			fbdefio->deferred_io = ufx_dpy_deferred_io;
		}
  // <<< BUG ANCHOR
		info->fbdefio = fbdefio;
		fb_deferred_io_init(info);
	}

	pr_debug("open /dev/fb%d user=%d fb_info=%p count=%d",
