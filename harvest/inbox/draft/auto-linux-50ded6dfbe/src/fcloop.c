// AUTO-DRAFT from torvalds/linux PR #a904dcf3fdf29cdac99ea354789bc5c82df0d5ad
	ret = nvmet_fc_rcv_ls_req(rport->targetport, rport,
				  &tls_req->ls_rsp,
				  lsreq->rqstaddr, lsreq->rqstlen);

	return ret;
}
