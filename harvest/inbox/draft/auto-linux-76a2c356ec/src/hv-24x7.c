// AUTO-DRAFT from torvalds/linux PR #7be4bc52fd9e0e55dc9a2e0232b89427d2c4b614
	attr->var = str;
	attr->attr.attr.name = name;
	attr->attr.attr.mode = 0444;
	attr->attr.show = device_show_string;  // <<< BUG ANCHOR

	return &attr->attr.attr;
}
