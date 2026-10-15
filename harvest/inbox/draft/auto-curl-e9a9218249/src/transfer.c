// AUTO-DRAFT from curl/curl PR #7190
return CURLE_OK;
}

#if defined(WIN32) && !defined(USE_LWIPSOCK)
#ifndef SIO_IDEAL_SEND_BACKLOG_QUERY
#define SIO_IDEAL_SEND_BACKLOG_QUERY 0x4004747B
#endif
