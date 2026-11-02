// AUTO-DRAFT from postgres/postgres PR #01131ca43bbf798be6ec237a33f6bef67bede888
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define HAVE_XMLSTRUCTUREDERRORCONTEXT 1
/* …（同文件无关代码省略）… */
#define PgXmlErrorPtr const xmlError *
/* …（同文件无关代码省略）… */
#define ERRCXT_MAGIC	68275028
/* …（同文件无关代码省略）… */
	PG_TRY();
	{
		size_t		decl_len = 0;

		xmlerrcxt = pg_xml_init(PG_XML_STRICTNESS_ALL);

/* …（同文件无关代码省略）… */
			xmlFreeNode(newline);
		}

		if (xmlSaveClose(ctxt) == -1 || xmlerrcxt->err_occurred)
		{
			ctxt = NULL;		/* don't try to close it again */
			xml_ereport(xmlerrcxt, ERROR, ERRCODE_INTERNAL_ERROR,
						"could not close xmlSaveCtxtPtr");
		}

		/*
		 * xmlDocContentDumpOutput may add a trailing newline, so remove that.
/* …（同文件无关代码省略）… */
void
pg_xml_init_library(void)
{
	static bool first_time = true;

	if (first_time)
	{
		/* Stuff we need do only once per session */

		/*
		 * Currently, we have no pure UTF-8 support for internals -- check if
		 * we can work.
		 */
		if (sizeof(char) != sizeof(xmlChar))
			ereport(ERROR,
					(errmsg("could not initialize XML library"),
					 errdetail("libxml2 has incompatible char type: sizeof(char)=%zu, sizeof(xmlChar)=%zu.",
							   sizeof(char), sizeof(xmlChar))));

#ifdef USE_LIBXMLCONTEXT
		/* Set up libxml's memory allocation our way */
		xml_memory_init();
#endif

		/* Check library compatibility */
		LIBXML_TEST_VERSION;

		first_time = false;
	}
}
/* …（同文件无关代码省略）… */
PgXmlErrorContext *
pg_xml_init(PgXmlStrictness strictness)
{
	PgXmlErrorContext *errcxt;
	void	   *new_errcxt;

	/* Do one-time setup if needed */
	pg_xml_init_library();

	/* Create error handling context structure */
	errcxt = palloc_object(PgXmlErrorContext);
	errcxt->magic = ERRCXT_MAGIC;
	errcxt->strictness = strictness;
	errcxt->err_occurred = false;
	initStringInfo(&errcxt->err_buf);

	/*
	 * Save original error handler and install ours. libxml originally didn't
	 * distinguish between the contexts for generic and for structured error
	 * handlers.  If we're using an old libxml version, we must thus save the
	 * generic error context, even though we're using a structured error
	 * handler.
	 */
	errcxt->saved_errfunc = xmlStructuredError;

#ifdef HAVE_XMLSTRUCTUREDERRORCONTEXT
	errcxt->saved_errcxt = xmlStructuredErrorContext;
#else
	errcxt->saved_errcxt = xmlGenericErrorContext;
#endif

	xmlSetStructuredErrorFunc(errcxt, xml_errorHandler);

	/*
	 * Verify that xmlSetStructuredErrorFunc set the context variable we
	 * expected it to.  If not, the error context pointer we just saved is not
	 * the correct thing to restore, and since that leaves us without a way to
	 * restore the context in pg_xml_done, we must fail.
	 *
	 * The only known situation in which this test fails is if we compile with
	 * headers from a libxml2 that doesn't track the structured error context
	 * separately (< 2.7.4), but at runtime use a version that does, or vice
	 * versa.  The libxml2 authors did not treat that change as constituting
	 * an ABI break, so the LIBXML_TEST_VERSION test in pg_xml_init_library
	 * fails to protect us from this.
	 */

#ifdef HAVE_XMLSTRUCTUREDERRORCONTEXT
	new_errcxt = xmlStructuredErrorContext;
#else
	new_errcxt = xmlGenericErrorContext;
#endif

	if (new_errcxt != errcxt)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("could not set up XML error handler"),
				 errhint("This probably indicates that the version of libxml2"
						 " being used is not compatible with the libxml2"
						 " header files that PostgreSQL was built with.")));

	/*
	 * Also, install an entity loader to prevent unwanted fetches of external
	 * files and URLs.
	 */
	errcxt->saved_entityfunc = xmlGetExternalEntityLoader();
	xmlSetExternalEntityLoader(xmlPgEntityLoader);

	return errcxt;
}
/* …（同文件无关代码省略）… */
static void
xml_memory_init(void)
{
	/* Create memory context if not there already */
	if (LibxmlContext == NULL)
		LibxmlContext = AllocSetContextCreate(TopMemoryContext,
											  "Libxml context",
											  ALLOCSET_DEFAULT_SIZES);

	/* Re-establish the callbacks even if already set */
	xmlMemSetup(xml_pfree, xml_palloc, xml_repalloc, xml_pstrdup);
}
/* …（同文件无关代码省略）… */
static void *
xml_palloc(size_t size)
{
	return MemoryContextAlloc(LibxmlContext, size);
}
/* …（同文件无关代码省略）… */
static void *
xml_repalloc(void *ptr, size_t size)
{
	return repalloc(ptr, size);
}
/* …（同文件无关代码省略）… */
static void
xml_pfree(void *ptr)
{
	/* At least some parts of libxml assume xmlFree(NULL) is allowed */
	if (ptr)
		pfree(ptr);
}
/* …（同文件无关代码省略）… */
static char *
xml_pstrdup(const char *string)
{
	return MemoryContextStrdup(LibxmlContext, string);
}
/* …（同文件无关代码省略）… */
static xmlParserInputPtr
xmlPgEntityLoader(const char *URL, const char *ID,
				  xmlParserCtxtPtr ctxt)
{
	return xmlNewStringInputStream(ctxt, (const xmlChar *) "");
}
/* …（同文件无关代码省略）… */
void
xml_ereport(PgXmlErrorContext *errcxt, int level, int sqlcode, const char *msg)
{
	char	   *detail;

	/* Defend against someone passing us a bogus context struct */
	if (errcxt->magic != ERRCXT_MAGIC)
		elog(ERROR, "xml_ereport called with invalid PgXmlErrorContext");

	/* Flag that the current libxml error has been reported */
	errcxt->err_occurred = false;

	/* Include detail only if we have some text from libxml */
	if (errcxt->err_buf.len > 0)
		detail = errcxt->err_buf.data;
	else
		detail = NULL;

	ereport(level,
			(errcode(sqlcode),
			 errmsg_internal("%s", msg),
			 detail ? errdetail_internal("%s", detail) : 0));
}
/* …（同文件无关代码省略）… */
static void
xml_errorHandler(void *data, PgXmlErrorPtr error)
{
	PgXmlErrorContext *xmlerrcxt = (PgXmlErrorContext *) data;
	xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) error->ctxt;
	xmlParserInputPtr input = (ctxt != NULL) ? ctxt->input : NULL;
	xmlNodePtr	node = error->node;
	const xmlChar *name = (node != NULL &&
						   node->type == XML_ELEMENT_NODE) ? node->name : NULL;
	int			domain = error->domain;
	int			level = error->level;
	StringInfoData errorBuf;

	/*
	 * Defend against 
