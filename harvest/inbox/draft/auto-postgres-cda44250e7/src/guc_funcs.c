// AUTO-DRAFT from postgres/postgres PR #470d9948965835a8c3bfb3ac102384bcc84a604c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void
GetConfigOptionValues(const struct config_generic *conf, const char **values)
{
	char		buffer[256];

	/* first get the generic attributes */

	/* name */
	values[0] = conf->name;

	/* setting: use ShowGUCOption in order to avoid duplicating the logic */
	values[1] = ShowGUCOption(conf, false);

	/* unit, if any (NULL is fine) */
	values[2] = get_config_unit_name(conf->flags);

	/* group */
	values[3] = _(config_group_names[conf->group]);

	/* short_desc */
	values[4] = conf->short_desc != NULL ? _(conf->short_desc) : NULL;

	/* extra_desc */
	values[5] = conf->long_desc != NULL ? _(conf->long_desc) : NULL;

	/* context */
	values[6] = GucContext_Names[conf->context];

	/* vartype */
	values[7] = config_type_names[conf->vartype];

	/* source */
	values[8] = GucSource_Names[conf->source];

	/* now get the type specific attributes */
	switch (conf->vartype)
	{
		case PGC_BOOL:
			{
				const struct config_bool *lconf = &conf->_bool;

				/* min_val */
				values[9] = NULL;

				/* max_val */
				values[10] = NULL;

				/* enumvals */
				values[11] = NULL;

				/* boot_val */
				values[12] = pstrdup(lconf->boot_val ? "on" : "off");

				/* reset_val */
				values[13] = pstrdup(lconf->reset_val ? "on" : "off");
			}
			break;

		case PGC_INT:
			{
				const struct config_int *lconf = &conf->_int;
				const char *fmt;

				/* choose display format */
				if (conf->flags & GUC_SHOW_IN_OCTAL)
					fmt = "0%03o";
				else
					fmt = "%d";

				/* min_val */
				snprintf(buffer, sizeof(buffer), fmt, lconf->min);
				values[9] = pstrdup(buffer);

				/* max_val */
				snprintf(buffer, sizeof(buffer), fmt, lconf->max);
				values[10] = pstrdup(buffer);

				/* enumvals */
				values[11] = NULL;

				/* boot_val */
				snprintf(buffer, sizeof(buffer), fmt, lconf->boot_val);
				values[12] = pstrdup(buffer);

				/* reset_val */
				snprintf(buffer, sizeof(buffer), fmt, lconf->reset_val);
				values[13] = pstrdup(buffer);
			}
			break;

		case PGC_REAL:
			{
				const struct config_real *lconf = &conf->_real;

				/* min_val */
				snprintf(buffer, sizeof(buffer), "%g", lconf->min);
				values[9] = pstrdup(buffer);

				/* max_val */
				snprintf(buffer, sizeof(buffer), "%g", lconf->max);
				values[10] = pstrdup(buffer);

				/* enumvals */
				values[11] = NULL;

				/* boot_val */
				snprintf(buffer, sizeof(buffer), "%g", lconf->boot_val);
				values[12] = pstrdup(buffer);

				/* reset_val */
				snprintf(buffer, sizeof(buffer), "%g", lconf->reset_val);
				values[13] = pstrdup(buffer);
			}
			break;

		case PGC_STRING:
			{
				const struct config_string *lconf = &conf->_string;

				/* min_val */
				values[9] = NULL;

				/* max_val */
				values[10] = NULL;

				/* enumvals */
				values[11] = NULL;

				/* boot_val */
				if (lconf->boot_val == NULL)
					values[12] = NULL;
				else
					values[12] = pstrdup(lconf->boot_val);

				/* reset_val */
				if (lconf->reset_val == NULL)
					values[13] = NULL;
				else
					values[13] = pstrdup(lconf->reset_val);
			}
			break;

		case PGC_ENUM:
			{
				const struct config_enum *lconf = &conf->_enum;

				/* min_val */
				values[9] = NULL;

				/* max_val */
				values[10] = NULL;

				/* enumvals */

				/*
				 * NOTE! enumvals with double quotes in them are not
				 * supported!
				 */
				values[11] = config_enum_get_options(lconf,
													 "{\"", "\"}", "\",\"");

				/* boot_val */
				values[12] = pstrdup(config_enum_lookup_by_value(conf,
																 lconf->boot_val));

				/* reset_val */
				values[13] = pstrdup(config_enum_lookup_by_value(conf,
																 lconf->reset_val));
			}
			break;

		default:
			{
				/*
				 * should never get here, but in case we do, set 'em to NULL
				 */

				/* min_val */
				values[9] = NULL;

				/* max_val */
				values[10] = NULL;

				/* enumvals */
				values[11] = NULL;

				/* boot_val */
				values[12] = NULL;

				/* reset_val */
				values[13] = NULL;
			}
			break;
	}

	/*
	 * If the setting came from a config file, set the source location. For
	 * security reasons, we don't show source file/line number for
	 * insufficiently-privileged users.
	 */
	if (conf->source == PGC_S_FILE &&
		has_privs_of_role(GetUserId(), ROLE_PG_READ_ALL_SETTINGS))
	{
		values[14] = conf->sourcefile;
		snprintf(buffer, sizeof(buffer), "%d", conf->sourceline);
		values[15] = pstrdup(buffer);
	}
	else
	{
		values[14] = NULL;
		values[15] = NULL;
	}

	values[16] = (conf->status & GUC_PENDING_RESTART) ? "t" : "f";
}
/* …（同文件无关代码省略）… */
#define NUM_PG_SETTINGS_ATTS	17
/* …（同文件无关代码省略）… */
	for (int i = 0; i < num_vars; i++)
	{
		struct config_generic *conf = guc_vars[i];
		char	   *values[NUM_PG_SETTINGS_ATTS];
		HeapTuple	tuple;

		/* skip if marked NO_SHOW_ALL or if not visible to current user */
/* …（同文件无关代码省略）… */
			continue;

		/* extract values for the current variable */
		GetConfigOptionValues(conf, (const char **) values);

		/* build a tuple and store it */
		tuple = BuildTupleFromCStrings(attinmeta, values);
		tuplestore_puttuple(rsinfo->setResult, tuple);
	}
