// AUTO-DRAFT from postgres/postgres PR #77705d1e5a616339c3e92f586bbfa7b6bb562beb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
Oid
get_object_oid_index(Oid class_id)
{
	const ObjectPropertyType *prop = get_object_property_data(class_id);

	return prop->oid_index_oid;
}
/* …（同文件无关代码省略）… */
SysCacheIdentifier
get_object_catcache_oid(Oid class_id)
{
	const ObjectPropertyType *prop = get_object_property_data(class_id);

	return prop->oid_catcache_id;
}
/* …（同文件无关代码省略）… */
HeapTuple
get_catalog_object_by_oid(Relation catalog, AttrNumber oidcol, Oid objectId)
{
	return
		get_catalog_object_by_oid_extended(catalog, oidcol, objectId, false);
}
/* …（同文件无关代码省略）… */
HeapTuple
get_catalog_object_by_oid_extended(Relation catalog,
								   AttrNumber oidcol,
								   Oid objectId,
								   bool locktup)
{
	HeapTuple	tuple;
	Oid			classId = RelationGetRelid(catalog);
	SysCacheIdentifier oidCacheId = get_object_catcache_oid(classId);

	if (oidCacheId >= 0)
	{
		if (locktup)
			tuple = SearchSysCacheLockedCopy1(oidCacheId,
											  ObjectIdGetDatum(objectId));
		else
			tuple = SearchSysCacheCopy1(oidCacheId,
										ObjectIdGetDatum(objectId));
		if (!HeapTupleIsValid(tuple))	/* should not happen */
			return NULL;
	}
	else
	{
		Oid			oidIndexId = get_object_oid_index(classId);
		SysScanDesc scan;
		ScanKeyData skey;

		Assert(OidIsValid(oidIndexId));

		ScanKeyInit(&skey,
					oidcol,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(objectId));

		scan = systable_beginscan(catalog, oidIndexId, true,
								  NULL, 1, &skey);
		tuple = systable_getnext(scan);
		if (!HeapTupleIsValid(tuple))
		{
			systable_endscan(scan);
			return NULL;
		}

		if (locktup)
			LockTuple(catalog, &tuple->t_self, InplaceUpdateTupleLock);

		tuple = heap_copytuple(tuple);

		systable_endscan(scan);
	}

	return tuple;
}
/* …（同文件无关代码省略）… */
static bool
getPublicationSchemaInfo(const ObjectAddress *object, bool missing_ok,
						 char **pubname, char **nspname)
{
	HeapTuple	tup;
	Form_pg_publication_namespace pnform;

	tup = SearchSysCache1(PUBLICATIONNAMESPACE,
						  ObjectIdGetDatum(object->objectId));
	if (!HeapTupleIsValid(tup))
	{
		if (!missing_ok)
			elog(ERROR, "cache lookup failed for publication schema %u",
				 object->objectId);
		return false;
	}

	pnform = (Form_pg_publication_namespace) GETSTRUCT(tup);
	*pubname = get_publication_name(pnform->pnpubid, missing_ok);
	if (!(*pubname))
	{
		ReleaseSysCache(tup);
		return false;
	}

	*nspname = get_namespace_name(pnform->pnnspid);
	if (!(*nspname))
	{
		Oid			schemaid = pnform->pnnspid;

		pfree(*pubname);
		ReleaseSysCache(tup);
		if (!missing_ok)
			elog(ERROR, "cache lookup failed for schema %u",
				 schemaid);
		return false;
	}

	ReleaseSysCache(tup);
	return true;
}
/* …（同文件无关代码省略）… */
	objidentity = getObjectIdentity(&address, true);

	/* schema name */
	if (OidIsValid(schema_oid) && objidentity)
	{
		const char *schema = quote_identifier(get_namespace_name(schema_oid));

		values[1] = CStringGetTextDatum(schema);
		nulls[1] = false;
	}
	else
		nulls[1] = true;

	/* object name */
	if (objname && objidentity)
/* …（同文件无关代码省略）… */
char *
getObjectIdentity(const ObjectAddress *object, bool missing_ok)
{
	return getObjectIdentityParts(object, NULL, NULL, missing_ok);
}
/* …（同文件无关代码省略）… */
char *
getObjectIdentityParts(const ObjectAddress *object,
					   List **objname, List **objargs,
					   bool missing_ok)
{
	StringInfoData buffer;

	initStringInfo(&buffer);

	/*
	 * Make sure that both objname and objargs were passed, or none was; and
	 * initialize them to empty lists.  For objname this is useless because it
	 * will be initialized in all cases inside the switch; but we do it anyway
	 * so that we can test below that no branch leaves it unset.
	 */
	Assert((objname != NULL) == (objargs != NULL));
	if (objname)
	{
		*objname = NIL;
		*objargs = NIL;
	}

	switch (object->classId)
	{
		case RelationRelationId:
			{
				char	   *attr = NULL;

				/*
				 * Check for the attribute first, so as if it is missing we
				 * can skip the entire relation description.
				 */
				if (object->objectSubId != 0)
				{
					attr = get_attname(object->objectId,
									   object->objectSubId,
									   missing_ok);

					if (missing_ok && attr == NULL)
						break;
				}

				getRelationIdentity(&buffer, object->objectId, objname,
									missing_ok);
				if (objname && *objname == NIL)
					break;

				if (attr)
				{
					appendStringInfo(&buffer, ".%s",
									 quote_identifier(attr));
					if (objname)
						*objname = lappend(*objname, attr);
				}
			}
			break;

		case ProcedureRelationId:
			{
				uint16		flags = FORMAT_PROC_FORCE_QUALIFY | FORMAT_PROC_INVALID_AS_NULL;
				char	   *proname = format_procedure_extended(object->objectId,
																flags);

				if (proname == NULL)
					break;

				appendStringInfoString(&buffer, proname);
				if (objname)
					format_procedure_parts(object->objectId, objname, objargs,
										   missing_ok);
				break;
			}

		case TypeRelationId:
			{
				uint16		flags = FORMAT_TYPE_INVALID_AS_NULL | FORMAT_TYPE_FORCE_QUALIFY;
				char	   *typeout;

				typeout = format_type_extended(object->objectId, -1, flags);

				if (typeout == NULL)
					break;

				appendStringInfoString(&buffer, typeout);
				if (objname)
					*objname = list_make1(typeout);
			}
			break;

		case CastRelationId:
			{
				Relation	castRel;
				HeapTuple	tup;
				Form_pg_cast castForm;

				castRel = table_open(CastRelationId, AccessShareLock);

				tup = get_catalog_object_by_oid(castRel, Anum_pg_cast_oid,
												object->objectId);

				if (!HeapTupleIsValid(tup))
				{
					if (!missing_ok)
						elog(ERROR, "could not find tuple for cast %u",
							 object->objectId);

					table_close(castRel, AccessShareLock);
					break;
				}

				castForm = (Form_pg_cast) GETSTRUCT(tup);

				appendStringInfo(&buffer, "(%s AS %s)",
								 format_type_be_qualified(castForm->castsource),
								 format_type_be_qualified(castForm->casttarget));

				if (objn
