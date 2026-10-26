// AUTO-DRAFT from postgres/postgres PR #54f36adcbcda8dec03e78c1cec453b9ebf3db3d2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define rl_completion_matches completion_matches
/* …（同文件无关代码省略）… */
#define COMPLETE_WITH_LIST_INT(cs, list) \
do { \
	completion_case_sensitive = (cs); \
	completion_charpp = (list); \
	matches = rl_completion_matches(text, complete_from_list); \
} while (0)
/* …（同文件无关代码省略）… */
#define COMPLETE_WITH_LIST(list) COMPLETE_WITH_LIST_INT(false, list)
/* …（同文件无关代码省略）… */
#define COMPLETE_WITH(...) \
do { \
	static const char *const list[] = { __VA_ARGS__, NULL }; \
	COMPLETE_WITH_LIST(list); \
} while (0)
/* …（同文件无关代码省略）… */
#define MatchAny  NULL
/* …（同文件无关代码省略）… */
static bool
word_matches(const char *pattern,
			 const char *word,
			 bool case_sensitive)
{
	size_t		wordlen;

#define cimatch(s1, s2, n) \
	(case_sensitive ? strncmp(s1, s2, n) == 0 : pg_strncasecmp(s1, s2, n) == 0)

	/* NULL pattern matches anything. */
	if (pattern == NULL)
		return true;

	/* Handle negated patterns from the MatchAnyExcept macro. */
	if (*pattern == '!')
		return !word_matches(pattern + 1, word, case_sensitive);

	/* Else consider each alternative in the pattern. */
	wordlen = strlen(word);
	for (;;)
	{
		const char *star = NULL;
		const char *c;

		/* Find end of current alternative, and locate any wild card. */
		c = pattern;
		while (*c != '\0' && *c != '|')
		{
			if (*c == '*')
				star = c;
			c++;
		}
		/* Was there a wild card? */
		if (star)
		{
			/* Yes, wildcard match? */
			size_t		beforelen = star - pattern,
						afterlen = c - star - 1;

			if (wordlen >= (beforelen + afterlen) &&
				cimatch(word, pattern, beforelen) &&
				cimatch(word + wordlen - afterlen, star + 1, afterlen))
				return true;
		}
		else
		{
			/* No, plain match? */
			if (wordlen == (c - pattern) &&
				cimatch(word, pattern, wordlen))
				return true;
		}
		/* Out of alternatives? */
		if (*c == '\0')
			break;
		/* Nope, try next alternative. */
		pattern = c + 1;
	}

	return false;
}
/* …（同文件无关代码省略）… */
static bool
TailMatchesArray(bool case_sensitive,
				 int previous_words_count, char **previous_words,
				 int narg, const char *const *args)
{
	if (previous_words_count < narg)
		return false;

	for (int argno = 0; argno < narg; argno++)
	{
		const char *arg = args[argno];

		if (!word_matches(arg, previous_words[narg - argno - 1],
						  case_sensitive))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool
HeadMatchesArray(bool case_sensitive,
				 int previous_words_count, char **previous_words,
				 int narg, const char *const *args)
{
	if (previous_words_count < narg)
		return false;

	for (int argno = 0; argno < narg; argno++)
	{
		const char *arg = args[argno];

		if (!word_matches(arg, previous_words[previous_words_count - argno - 1],
						  case_sensitive))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool
MatchesArray(bool case_sensitive,
			 int previous_words_count, char **previous_words,
			 int narg, const char *const *args)
{
	int			match_any_pos = -1;

	/* Even with MatchAnyN, there must be at least N-1 words */
	if (previous_words_count < narg - 1)
		return false;

	/* Check for MatchAnyN */
	for (int argno = 0; argno < narg; argno++)
	{
		const char *arg = args[argno];

		if (arg != NULL && arg[0] == '\0')
		{
			match_any_pos = argno;
			break;
		}
	}

	if (match_any_pos < 0)
	{
		/* Standard case without MatchAnyN */
		if (previous_words_count != narg)
			return false;

		/* Either Head or Tail match will do for the rest */
		if (!HeadMatchesArray(case_sensitive,
							  previous_words_count, previous_words,
							  narg, args))
			return false;
	}
	else
	{
		/* Match against head */
		if (!HeadMatchesArray(case_sensitive,
							  previous_words_count, previous_words,
							  match_any_pos, args))
			return false;

		/* Match against tail */
		if (!TailMatchesArray(case_sensitive,
							  previous_words_count, previous_words,
							  narg - match_any_pos - 1,
							  args + match_any_pos + 1))
			return false;
	}

	return true;
}
/* …（同文件无关代码省略）… */
static bool
MatchesImpl(bool case_sensitive,
			int previous_words_count, char **previous_words,
			int narg, ...)
{
	const char *argarray[64];
	va_list		args;

	Assert(narg <= lengthof(argarray));

	/* Even with MatchAnyN, there must be at least N-1 words */
	if (previous_words_count < narg - 1)
		return false;

	va_start(args, narg);
	for (int argno = 0; argno < narg; argno++)
		argarray[argno] = va_arg(args, const char *);
	va_end(args);

	return MatchesArray(case_sensitive,
						previous_words_count, previous_words,
						narg, argarray);
}
/* …（同文件无关代码省略）… */
#define Matches(...) \
	MatchesImpl(false, previous_words_count, previous_words, \
				VA_ARGS_NARGS(__VA_ARGS__), __VA_ARGS__)
/* …（同文件无关代码省略）… */
	else if (Matches("REFRESH", "MATERIALIZED", "VIEW", MatchAny, "WITH"))
		COMPLETE_WITH("NO DATA", "DATA");
	else if (Matches("REFRESH", "MATERIALIZED", "VIEW", "CONCURRENTLY", MatchAny, "WITH"))
		COMPLETE_WITH("NO DATA", "DATA");
	else if (Matches("REFRESH", "MATERIALIZED", "VIEW", MatchAny, "WITH", "NO"))
		COMPLETE_WITH("DATA");
	else if (Matches("REFRESH", "MATERIALIZED", "VIEW", "CONCURRENTLY", MatchAny, "WITH", "NO"))
		COMPLETE_WITH("DATA");

/* REINDEX */
/* …（同文件无关代码省略）… */
static char *
complete_from_list(const char *text, int state)
{
	static int	string_length,
				list_index,
				matches;
	static bool casesensitive;
	const char *item;

	/* need to have a list */
	Assert(completion_charpp != NULL);

	/* Initialization */
	if (state == 0)
	{
		list_index = 0;
		string_length = strlen(text);
		casesensitive = completion_case_sensitive;
		matches = 0;
	}

	while ((item = completion_charpp[list_index++]))
	{
		/* First pass is case sensitive */
		if (casesensitive && strncmp(text, item, string_length) == 0)
		{
			matches++;
			return pg_strdup(item);
		}

		/* Second pass is case insensitive, don't bother counting matches */
		if (!casesensitive && pg_strncasec
