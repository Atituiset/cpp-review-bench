// AUTO-DRAFT from torvalds/linux PR #d77bd0d469e1923e587bb919301c0bb29093e30a
	if (of_property_read_u32(np, "maxim,over-volt", &prop))
		chip->vmax = INT_MAX;
	else
		chip->vmin = prop;  // <<< BUG ANCHOR

	return 0;
}
