// AUTO-DRAFT from curl/curl PR #7552
#define HAVE_CARES_CALLBACK_TIMEOUTS 1
#endif
  // <<< BUG ANCHOR
/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

struct thread_data {
  int num_pending; /* number of ares_gethostbyname() requests */
  struct Curl_addrinfo *temp_ai; /* intermediary result while fetching c-ares
                                    parts */
  int last_status;
  return result;
}

/* Connects results to the list */
static void compound_results(struct thread_data *res,
                             struct Curl_addrinfo *ai)
    }
  }
}

/*
 * Curl_resolver_getaddrinfo() - when using ares
 *
    /* initial status - failed */
    res->last_status = ARES_ENOTFOUND;

#if ARES_VERSION >= 0x010601
    /* IPv6 supported by c-ares since 1.6.1 */
    if(Curl_ipv6works(data)) {
      /* The stack seems to be IPv6-enabled */
      res->num_pending = 2;
                          PF_INET6, query_completed_cb, data);
    }
    else
#endif /* ARES_VERSION >= 0x010601 */
    {
      res->num_pending = 1;

                         hostname, PF_INET,
                         query_completed_cb, data);
    }

    *waitp = 1; /* expect asynchronous response */
  }
  return NULL; /* no struct yet */
  if(!(servers && servers[0]))
    return CURLE_OK;

#if (ARES_VERSION >= 0x010704)
#if (ARES_VERSION >= 0x010b00)
  ares_result = ares_set_servers_ports_csv(data->state.async.resolver,
                                           servers);
#else
CURLcode Curl_set_dns_interface(struct Curl_easy *data,
                                const char *interf)
{
#if (ARES_VERSION >= 0x010704)
  if(!interf)
    interf = "";

CURLcode Curl_set_dns_local_ip4(struct Curl_easy *data,
                                const char *local_ip4)
{
#if (ARES_VERSION >= 0x010704)
  struct in_addr a4;

  if((!local_ip4) || (local_ip4[0] == 0)) {
CURLcode Curl_set_dns_local_ip6(struct Curl_easy *data,
                                const char *local_ip6)
{
#if (ARES_VERSION >= 0x
