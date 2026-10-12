// AUTO-DRAFT from torvalds/linux PR #d6d3455d8751854e0d1820f14184682e23ac4475
			 * to properly handle merged rulesets without
			 * overlapping access rights.  These layers are set once
			 * and never changed for the lifetime of the domain.
			 */
			struct access_masks  // <<< BUG ANCHOR
				handled_masks[] __counted_by(num_layers);
		};
	};
};
