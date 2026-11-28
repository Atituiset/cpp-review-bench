// AUTO-DRAFT from torvalds/linux PR #ad5b48d7188721ba3a7a351d06a78f8030148e61
		 *
		 * The difference between "aquaero Device" and the other two is in the collections
		 * they present. The two other devices have the type of the second element in
		 * their respective collections set to 1, while the real device has it set to 0.
		 */
		if (hdev->maxcollection < 2 || hdev->collection[1].type != 0) {  // <<< BUG ANCHOR
			ret = -ENODEV;
			goto fail_and_close;
		}
