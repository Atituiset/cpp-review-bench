// AUTO-DRAFT from torvalds/linux PR #4375b45d7060a6a3f41e14a9181c5c913ea474a4

	if (nsid != U32_MAX) {
		status = nvmet_req_find_ns(req);
		if (status)
			return status;
		if (!req->ns->pr.enable)
/* …（同文件无关代码省略）… */
	u16 status;

	status = nvmet_req_find_ns(req);
	if (status)
		return status;
