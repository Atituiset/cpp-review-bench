// AUTO-DRAFT from torvalds/linux PR #8a65162514fbad21a2c58ffed65f42b27e92b163
		struct kernfs_iattrs *attrs = READ_ONCE(parent->iattr);
  // <<< BUG ANCHOR
		if (attrs)
			gid = READ_ONCE(attrs->ia_gid);

		if (flags & KERNFS_DIR)
			mode |= S_ISGID;
