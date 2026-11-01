// AUTO-DRAFT from postgres/postgres PR #77705d1e5a616339c3e92f586bbfa7b6bb562beb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define NO_XML_SUPPORT() \
	ereport(ERROR, \
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
			 errmsg("unsupported XML feature"), \
			 errdetail("This functionality requires the server to be built with libxml support.")))
/* …（同文件无关代码省略）… */
#define NAMESPACE_XSI "http://www.w3.org/2001/XMLSchema-instance"
/* …（同文件无关代码省略）… */
static pg_wchar
sqlchar_to_unicode(const char *s)
{
	char	   *utf8string;
	pg_wchar	ret[2];			/* need space for trailing zero */

	utf8string = pg_server_to_any(s, pg_mblen_cstr(s), PG_UTF8);

	pg_encoding_mb2wchar_with_len(PG_UTF8, utf8string, ret,
								  pg_encoding_mblen(PG_UTF8, utf8string));

	if (utf8string != s)
		pfree(utf8string);

	return ret[0];
}
/* …（同文件无关代码省略）… */
static bool
is_valid_xml_namefirst(pg_wchar c)
{
	/* (Letter | '_' | ':') */
	return (xmlIsBaseCharQ(c) || xmlIsIdeographicQ(c)
			|| c == '_' || c == ':');
}
/* …（同文件无关代码省略）… */
static bool
is_valid_xml_namechar(pg_wchar c)
{
	/* Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender */
	return (xmlIsBaseCharQ(c) || xmlIsIdeographicQ(c)
			|| xmlIsDigitQ(c)
			|| c == '.' || c == '-' || c == '_' || c == ':'
			|| xmlIsCombiningQ(c)
			|| xmlIsExtenderQ(c));
}
/* …（同文件无关代码省略）… */
char *
map_sql_identifier_to_xml_name(const char *ident, bool fully_escaped,
							   bool escape_period)
{
#ifdef USE_LIBXML
	StringInfoData buf;
	const char *p;

	/*
	 * SQL/XML doesn't make use of this case anywhere, so it's probably a
	 * mistake.
	 */
	Assert(fully_escaped || !escape_period);

	initStringInfo(&buf);

	for (p = ident; *p; p += pg_mblen_cstr(p))
	{
		if (*p == ':' && (p == ident || fully_escaped))
			appendStringInfoString(&buf, "_x003A_");
		else if (*p == '_' && *(p + 1) == 'x')
			appendStringInfoString(&buf, "_x005F_");
		else if (fully_escaped && p == ident &&
				 pg_strncasecmp(p, "xml", 3) == 0)
		{
			if (*p == 'x')
				appendStringInfoString(&buf, "_x0078_");
			else
				appendStringInfoString(&buf, "_x0058_");
		}
		else if (escape_period && *p == '.')
			appendStringInfoString(&buf, "_x002E_");
		else
		{
			pg_wchar	u = sqlchar_to_unicode(p);

			if ((p == ident)
				? !is_valid_xml_namefirst(u)
				: !is_valid_xml_namechar(u))
				appendStringInfo(&buf, "_x%04X_", (unsigned int) u);
			else
				appendBinaryStringInfo(&buf, p, pg_mblen_cstr(p));
		}
	}

	return buf.data;
#else							/* not USE_LIBXML */
	NO_XML_SUPPORT();
	return NULL;
#endif							/* not USE_LIBXML */
}
/* …（同文件无关代码省略）… */
static void
xmldata_root_element_start(StringInfo result, const char *eltname,
						   const char *xmlschema, const char *targetns,
						   bool top_level)
{
	/* This isn't really wrong but currently makes no sense. */
	Assert(top_level || !xmlschema);

	appendStringInfo(result, "<%s", eltname);
	if (top_level)
	{
		appendStringInfoString(result, " xmlns:xsi=\"" NAMESPACE_XSI "\"");
		if (strlen(targetns) > 0)
			appendStringInfo(result, " xmlns=\"%s\"", targetns);
	}
	if (xmlschema)
	{
		/* FIXME: better targets */
		if (strlen(targetns) > 0)
			appendStringInfo(result, " xsi:schemaLocation=\"%s #\"", targetns);
		else
			appendStringInfoString(result, " xsi:noNamespaceSchemaLocation=\"#\"");
	}
	appendStringInfoString(result, ">\n");
}
/* …（同文件无关代码省略）… */
					   bool tableforest, const char *targetns, bool top_level)
{
	StringInfo	result;
	char	   *xmlsn;
	List	   *relid_list;
	ListCell   *cell;

	xmlsn = map_sql_identifier_to_xml_name(get_namespace_name(nspid),
										   true, false);
	result = makeStringInfo();

	xmldata_root_element_start(result, xmlsn, xmlschema, targetns, top_level);
/* …（同文件无关代码省略）… */
static char *
map_multipart_sql_identifier_to_xml_name(const char *a, const char *b, const char *c, const char *d)
{
	StringInfoData result;

	initStringInfo(&result);

	if (a)
		appendStringInfoString(&result,
							   map_sql_identifier_to_xml_name(a, true, true));
	if (b)
		appendStringInfo(&result, ".%s",
						 map_sql_identifier_to_xml_name(b, true, true));
	if (c)
		appendStringInfo(&result, ".%s",
						 map_sql_identifier_to_xml_name(c, true, true));
	if (d)
		appendStringInfo(&result, ".%s",
						 map_sql_identifier_to_xml_name(d, true, true));

	return result.data;
}
/* …（同文件无关代码省略）… */
	dbname = get_database_name(MyDatabaseId);
	nspname = get_namespace_name(nspid);

	initStringInfo(&result);

	xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
/* …（同文件无关代码省略）… */
	{
		Oid			nspid = lfirst_oid(cell);
		char	   *nspname = get_namespace_name(nspid);
		char	   *xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
		char	   *schematypename = map_multipart_sql_identifier_to_xml_name("SchemaType",
																			  dbname,
																			  nspname,
																			  NULL);

		appendStringInfo(&result,
						 "    <xsd:element name=\"%s\" type=\"%s\"/>\n",
