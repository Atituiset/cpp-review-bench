// AUTO-DRAFT from torvalds/linux PR #eef2bc5a82c25d25c3082f5d86d97a300caeb678
static inline int cap_capable_helper(const struct cred *cred,
				     struct user_namespace *target_ns,
				     const struct user_namespace *cred_ns,
				     int cap)
{
	struct user_namespace *ns = target_ns;
  // <<< BUG ANCHOR
	/* See if cred has the capability in the target user namespace
	 * by examining the target user namespace and all of the target
	 * user namespace's parents.
	 */
	for (;;) {
		/* Do we have the necessary capabilities? */
		if (likely(ns == cred_ns))
			return cap_raised(cred->cap_effective, cap) ? 0 : -EPERM;

		/*
		 * If we're already at a lower level than we're looking for,
		 * we're done searching.
		 */
		if (ns->level <= cred_ns->level)
			return -EPERM;

		/* 
		 * The owner of the user namespace in the parent of the
		 * user namespace has all caps.
		 */
		if ((ns->parent == cred_ns) && uid_eq(ns->owner, cred->euid))
			return 0;

		/*
		 * If you have a capability in a parent user ns, then you have
		 * it over all children user namespaces as well.
		 */
		ns = ns->parent;
	}

	/* We never get here */
}
/* …（同文件无关代码省略）… */
int cap_capable(const struct cred *cred, struct user_namespace *target_ns,
		int cap, unsigned int opts)
{
	const struct user_namespace *cred_ns = cred->user_ns;
	int ret = cap_capable_helper(cred, target_ns, cred_ns, cap);

	trace_cap_capable(cred, target_ns, cred_ns, cap, ret);
	return ret;
}
/* …（同文件无关代码省略）… */
	return commit_creds(new);
}

/**
 * cap_task_prctl - Implement process control functions for this security module
 * @option: The process control function requested
/* …（同文件无关代码省略）… */
			/* cannot change a locked bit */
			return -EPERM;

		/*
		 * Doing anything requires privilege (go read about the
		 * "sendmail capabilities bug"), except for unprivileged bits.
		 * Indeed, the SECURE_ALL_UNPRIVILEGED bits are not
		 * restrictions enforced by the kernel but by user space on
		 * itself.
		 */
		if (cap_capable(current_cred(), current_cred()->user_ns,
				CAP_SETPCAP, CAP_OPT_NONE) != 0) {
			const unsigned long unpriv_and_locks =
				SECURE_ALL_UNPRIVILEGED |
				SECURE_ALL_UNPRIVILEGED << 1;
			const unsigned long changed = old->securebits ^ arg2;

			/* For legacy reason, denies non-change. */
			if (!changed)
				return -EPERM;

			/* Denies privileged changes. */
			if (changed & ~unpriv_and_locks)
				return -EPERM;
		}

		new = prepare_creds();
		if (!new)
