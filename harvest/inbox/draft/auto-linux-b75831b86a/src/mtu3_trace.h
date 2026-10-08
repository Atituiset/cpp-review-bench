// AUTO-DRAFT from torvalds/linux PR #8fc6e594884a65ec99c78ad05e7fae110ad714d9
		__field(unsigned int, flags)
		__field(unsigned int, direction)
		__field(struct mtu3_gpd_ring *, gpd_ring)
	),
	TP_fast_assign(
		__assign_str(name);
/* …（同文件无关代码省略）… */
		__entry->flags = mep->flags;
		__entry->direction = mep->is_in;
		__entry->gpd_ring = &mep->gpd_ring;
	),
	TP_printk("%s: type %s maxp %d slot %d mult %d burst %d ring %p/%pad flags %c:%c%c%c:%c",
		__get_str(name), usb_ep_type_string(__entry->type),
		__entry->maxp, __entry->slot,
		__entry->mult, __entry->maxburst,
		__entry->gpd_ring, &__entry->gpd_ring->dma,  // <<< BUG ANCHOR
		__entry->flags & MTU3_EP_ENABLED ? 'E' : 'e',
		__entry->flags & MTU3_EP_STALL ? 'S' : 's',
		__entry->flags & MTU3_EP_WEDGE ? 'W' : 'w',
