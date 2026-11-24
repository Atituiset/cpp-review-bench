// AUTO-DRAFT from postgres/postgres PR #6a0a66b3e7ac394c499708b790effea9a564ef0d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
}

/*
 * Common checks for altering failover, two_phase, and retain_dead_tuples
 * options.
 */
static void
CheckAlterSubOption(Subscription *sub, const char *option,
					bool slot_needs_update, bool isTopLevel)
{
	Assert(strcmp(option, "failover") == 0 ||
		   strcmp(option, "two_phase") == 0 ||
		   strcmp(option, "retain_dead_tuples") == 0);

	/*
	 * Altering the retain_dead_tuples option does not update the slot on the
	 * publisher.
	 */
	Assert(!slot_needs_update || strcmp(option, "retain_dead_tuples") != 0);

	/*
	 * Do not allow changing the option if the subscription is enabled. This
/* …（同文件无关代码省略）… */
	 * avoid the race conditions described above, but we maintain the
	 * restriction for both enable and disable operations for the sake of
	 * consistency.
	 */
	if (sub->enabled)
		ereport(ERROR,
/* …（同文件无关代码省略）… */
						bool		update_relid;
						Oid			relid = InvalidOid;

						values[Anum_pg_subscription_subconflictlogdest - 1] =
							CStringGetTextDatum(ConflictLogDestNames[opts.conflictlogdest]);
						replaces[Anum_pg_subscription_subconflictlogdest - 1] = true;
/* …（同文件无关代码省略）… */
	Oid			subowner;
	Oid			subserver;
	Oid			subconflictlogrelid;
	char	   *subconninfo = NULL;
	Datum		datum;
	bool		isnull;
/* …（同文件无关代码省略）… */
	subowner = form->subowner;
	subserver = form->subserver;
	subconflictlogrelid = form->subconflictlogrelid;
	must_use_password = !superuser_arg(subowner) && form->subpasswordrequired;

	/* must be owner */
/* …（同文件无关代码省略）… */
	}
	list_free(subworkers);

	/*
	 * Remove the no-longer-useful entry in the launcher's table of apply
	 * worker start times.
/* …（同文件无关代码省略）… */
{
	Form_pg_subscription form;
	AclResult	aclresult;

	form = (Form_pg_subscription) GETSTRUCT(tup);

/* …（同文件无关代码省略）… */
	if (OidIsValid(form->subserver))
		GetUserMappingExtended(newOwnerId, form->subserver, WARNING);

	form->subowner = newOwnerId;
	CatalogTupleUpdate(rel, &tup->t_self, tup);

	/* Update owner of the conflict log table if it exists. */
	if (OidIsValid(form->subconflictlogrelid))
		ATExecChangeOwner(form->subconflictlogrelid, newOwnerId, true,
						  AccessExclusiveLock);

	/* Update owner dependency reference */
	changeDependencyOnOwner(SubscriptionRelationId,
							form->oid,
/* …（同文件无关代码省略）… */
	/* Wake up related background processes to handle this change quickly. */
	ApplyLauncherWakeupAtCommit();
	LogicalRepWorkersWakeupAtCommit(form->oid);
}

/*
