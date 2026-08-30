// AUTO-DRAFT from nginx/nginx PR #411
return NGX_ERROR;
    }

    if (!ngx_quic_supported_version(pkt->version)) {
        return NGX_ABORT;
    }
