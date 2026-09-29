// AUTO-DRAFT from postgres/postgres PR #d9b5a63f49d9d372a6609243eb0e9417a0a35e6f
		S_UNLOCK(&struct_w_lock.lock);
  // <<< BUG ANCHOR
		/* and that "contended" acquisition works */
		s_lock(&struct_w_lock.lock, "testfile", 17, "testfunc");
		S_UNLOCK(&struct_w_lock.lock);

		/*
