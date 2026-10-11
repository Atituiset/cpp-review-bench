// AUTO-DRAFT from torvalds/linux PR #c61e23ed14359e6289c08e55940dd3401fe27db0
	for_each_new_connector_in_state(state, connector, connector_state, i) {
		if (connector_state->crtc)
			break;
	}

	/*
