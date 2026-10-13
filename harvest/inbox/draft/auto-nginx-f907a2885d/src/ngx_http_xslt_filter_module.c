// AUTO-DRAFT from nginx/nginx PR #1565
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

#define NGX_HTTP_XSLT_REUSE_DTD  1
/* …（同文件无关代码省略）… */
typedef struct {
    xmlDtdPtr                  dtd;
    ngx_array_t                sheets;       /* ngx_http_xslt_sheet_t */
    ngx_hash_t                 types;
    ngx_array_t               *types_keys;
    ngx_array_t               *params;       /* ngx_http_xslt_param_t */
    ngx_flag_t                 last_modified;
    ngx_flag_t                 external_entities;
} ngx_http_xslt_filter_loc_conf_t;
/* …（同文件无关代码省略）… */
typedef struct {
    xmlDocPtr                  doc;
    xmlParserCtxtPtr           ctxt;
    xsltTransformContextPtr    transform;
    ngx_http_request_t        *request;
    ngx_array_t                params;

    ngx_uint_t                 done;         /* unsigned  done:1; */
} ngx_http_xslt_filter_ctx_t;
/* …（同文件无关代码省略）… */

        if (ngx_http_xslt_add_chunk(r, ctx, cl->buf) != NGX_OK) {

            if (ctx->ctxt->myDoc) {

#if (NGX_HTTP_XSLT_REUSE_DTD)
/* …（同文件无关代码省略）… */
static ngx_int_t
ngx_http_xslt_add_chunk(ngx_http_request_t *r, ngx_http_xslt_filter_ctx_t *ctx,
    ngx_buf_t *b)
{
    int               err;
    xmlParserCtxtPtr  ctxt;

    if (ctx->ctxt == NULL) {

        ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);
        if (ctxt == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "xmlCreatePushParserCtxt() failed");
            return NGX_ERROR;
        }
        xmlCtxtUseOptions(ctxt, XML_PARSE_NOENT|XML_PARSE_DTDLOAD
                                |XML_PARSE_NONET|XML_PARSE_NOWARNING);

        ctxt->sax->externalSubset = ngx_http_xslt_sax_external_subset;
        ctxt->sax->entityDecl = ngx_http_xslt_sax_entity_decl;
        ctxt->sax->setDocumentLocator = NULL;
        ctxt->sax->error = ngx_http_xslt_sax_error;
        ctxt->sax->fatalError = ngx_http_xslt_sax_error;
        ctxt->sax->_private = ctx;

        ctx->ctxt = ctxt;
        ctx->request = r;
    }

    err = xmlParseChunk(ctx->ctxt, (char *) b->pos, (int) (b->last - b->pos),
                        (b->last_buf) || (b->last_in_chain));

    if (err == 0) {
        b->pos = b->last;
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "xmlParseChunk() failed, error:%d", err);

    return NGX_ERROR;
}
/* …（同文件无关代码省略）… */
static void
ngx_http_xslt_sax_external_subset(void *data, const xmlChar *name,
    const xmlChar *externalId, const xmlChar *systemId)
{
    xmlParserCtxtPtr ctxt = data;

    xmlDocPtr                         doc;
    xmlDtdPtr                         dtd;
    ngx_http_request_t               *r;
    ngx_http_xslt_filter_ctx_t       *ctx;
    ngx_http_xslt_filter_loc_conf_t  *conf;

    ctx = ctxt->sax->_private;
    r = ctx->request;

    conf = ngx_http_get_module_loc_conf(r, ngx_http_xslt_filter_module);

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "xslt filter extSubset: \"%s\" \"%s\" \"%s\"",
                   name ? name : (xmlChar *) "",
                   externalId ? externalId : (xmlChar *) "",
                   systemId ? systemId : (xmlChar *) "");

    doc = ctxt->myDoc;

#if (NGX_HTTP_XSLT_REUSE_DTD)

    dtd = conf->dtd;

#else

    dtd = xmlCopyDtd(conf->dtd);
    if (dtd == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "xmlCopyDtd() failed");
        return;
    }

    if (doc->children == NULL) {
        xmlAddChild((xmlNodePtr) doc, (xmlNodePtr) dtd);

    } else {
        xmlAddPrevSibling(doc->children, (xmlNodePtr) dtd);
    }

#endif

    doc->extSubset = dtd;
}
/* …（同文件无关代码省略）… */
static void
ngx_http_xslt_sax_entity_decl(void *data, const xmlChar *name, int type,
    const xmlChar *publicId, const xmlChar *systemId, xmlChar *content)
{
    xmlParserCtxtPtr ctxt = data;

    ngx_http_request_t               *r;
    ngx_http_xslt_filter_ctx_t       *ctx;
    ngx_http_xslt_filter_loc_conf_t  *conf;

    ctx = ctxt->sax->_private;
    r = ctx->request;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "xslt filter entityDecl: \"%s\" \"%s\" \"%s\"",
                   name ? name : (xmlChar *) "",
                   publicId ? publicId : (xmlChar *) "",
                   systemId ? systemId : (xmlChar *) "");

    conf = ngx_http_get_module_loc_conf(r, ngx_http_xslt_filter_module);

    if (systemId && !conf->external_entities) {

        /*
         * If external entiries in the internal DTD subset are disabled,
         * we remove system identifiers from such entities.  This makes sure
         * that external entities cannot be used to directly request arbitrary
         * files, but still can be used with public identifiers, assuming these
         * are included into XML catalogs on the system.
         */

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "xslt filter external entity ignored: "
                      "\"%s\" \"%s\" \"%s\"",
                      name ? name : (xmlChar *) "",
                      publicId ? publicId : (xmlChar *) "",
                      systemId ? systemId : (xmlChar *) "");

        if (publicId) {
            xmlSAX2EntityDecl(data, name, type, publicId, (xmlChar *) "",
                              content);

        } else if (type == XML_EXTERNAL_GENERAL_PARSED_ENTITY) {
            xmlSAX2EntityDecl(data, name, XML_INTERNAL_GENERAL_ENTITY,
                              NULL, NULL, (xmlChar *) "");

        } else if (type == XML_EXTERNAL_PARAMETER_ENTITY) {
            xmlSAX2EntityDecl(data, name, XML_INTERNAL_PARAMETER_ENTITY,
                              NULL, NULL, (xmlChar *) "");
        }

        return;
    }

    xmlSAX2EntityDecl(data, name, type, publicId, systemId, content);
}
/* …（同文件无关代码省略）… */
static void ngx_cdecl
ngx_http_xslt_sax_error(void *data, const char *msg, ...)
{
    xmlParserCtxtPtr ctxt = 
