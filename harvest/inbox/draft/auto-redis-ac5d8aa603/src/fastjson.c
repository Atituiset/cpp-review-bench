// AUTO-DRAFT from redis/redis PR #15804
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#include <ctype.h>
#include <string.h>

// Forward declarations.
static int jsonSkipValue(const char **p, const char *end);
static exprtoken *jsonParseValueToken(const char **p, const char *end);

/* Similar to ctype.h isdigit() but covers the whole JSON number charset,
 * including exp form. */
static int jsonIsNumberChar(int c) {
    return isdigit(c) || c=='-' || c=='+' || c=='.' || c=='e' || c=='E';
}
/* …（同文件无关代码省略）… */
static inline void jsonSkipWhiteSpaces(const char **p, const char *end) {
    while (*p < end && isspace((unsigned char)**p)) (*p)++;
}
/* …（同文件无关代码省略）… */
static int jsonSkipString(const char **p, const char *end) {
    if (*p >= end || **p != '"') return 0;
    (*p)++; /* Skip opening quote. */
    while (*p < end) {
        if (**p == '\\') {
            (*p) += 2;
            continue;
        }
        if (**p == '"') {
            (*p)++; /* Skip closing quote. */
            return 1;
        }
        (*p)++;
    }
    return 0; /* unterminated */
}
/* …（同文件无关代码省略）… */
static int jsonSkipBracketed(const char **p, const char *end,
                             char opener, char closer) {
    int depth = 1;
    (*p)++; /* Skip opener. */

    /* Loop until we reach the end of the input or find the matching
     * closer (depth becomes 0). */
    while (*p < end && depth > 0) {
        char c = **p;

        if (c == '"') {
            // Found a string, delegate skipping to jsonSkipString().
            if (!jsonSkipString(p, end)) {
                return 0; // String skipping failed (e.g., unterminated)
            }
            /* jsonSkipString() advances *p past the closing quote.
             * Continue the loop to process the character *after* the string. */
            continue;
        }

        /* If it's not a string, check if it affects the depth for the
         * specific brackets we are currently tracking. */
        if (c == opener) {
            depth++;
        } else if (c == closer) {
            depth--;
        }

        /* Always advance the pointer for any non-string character.
         * This handles commas, colons, whitespace, numbers, literals,
         * and even nested brackets of a *different* type than the
         * one we are currently skipping (e.g. skipping a { inside []). */
        (*p)++;
    }

    /* Return 1 (true) if we successfully found the matching closer,
     * otherwise there is a parse error and we return 0. */
    return depth == 0;
}
/* …（同文件无关代码省略）… */
static int jsonSkipLiteral(const char **p, const char *end, const char *lit) {
    size_t l = strlen(lit);
    if (*p + l > end) return 0;
    if (strncmp(*p, lit, l) == 0) { *p += l; return 1; }
    return 0;
}
/* …（同文件无关代码省略）… */
static int jsonSkipNumber(const char **p, const char *end) {
    const char *num_start = *p;
    while (*p < end && jsonIsNumberChar(**p)) (*p)++;
    return *p > num_start; // Any progress made? Otherwise no number found.
}
/* …（同文件无关代码省略）… */
static int jsonSkipValue(const char **p, const char *end) {
    jsonSkipWhiteSpaces(p, end);
    if (*p >= end) return 0;
    switch (**p) {
    case '"': return jsonSkipString(p, end);
    case '{':  return jsonSkipBracketed(p, end, '{', '}');
    case '[':  return jsonSkipBracketed(p, end, '[', ']');
    case 't':  return jsonSkipLiteral(p, end, "true");
    case 'f':  return jsonSkipLiteral(p, end, "false");
    case 'n':  return jsonSkipLiteral(p, end, "null");
    default: return jsonSkipNumber(p, end);
    }
}
/* …（同文件无关代码省略）… */
static exprtoken *jsonParseStringToken(const char **p, const char *end) {
    if (*p >= end || **p != '"') return NULL;
    const char *start = ++(*p);
    int esc = 0; size_t len = 0; int has_esc = 0;
    const char *q = *p;
    while (q < end) {
        if (esc) { esc = 0; q++; len++; has_esc = 1; continue; }
        if (*q == '\\') { esc = 1; q++; continue; }
        if (*q == '"') break;
        q++; len++;
    }
    if (q >= end || *q != '"') return NULL; // Unterminated string
    exprtoken *t = exprNewToken(EXPR_TOKEN_STR);

    if (!has_esc) {
        // No escapes, we can point directly into the original JSON string.
        t->str.start = (char*)start; t->str.len = len; t->str.heapstr = NULL;
    } else {
        // Escapes present, need to allocate and copy/process escapes.
        char *dst = RedisModule_Alloc(len + 1);

        t->str.start = t->str.heapstr = dst; t->str.len = len;
        const char *r = start; esc = 0;
        while (r < q) {
            if (esc) {
                switch (*r) {
                // Supported escapes from Goal 3.
                case 'n': *dst='\n'; break;
                case 'r': *dst='\r'; break;
                case 't': *dst='\t'; break;
                case '\\': *dst='\\'; break;
                case '"': *dst='\"'; break;
                // Escapes (like \uXXXX, \b, \f) are not supported for now,
                // we just copy them verbatim.
                default: *dst=*r; break;
                }
                dst++; esc = 0; r++; continue;
            }
            if (*r == '\\') { esc = 1; r++; continue; }
            *dst++ = *r++;
        }
        *dst = '\0'; // Null-terminate the allocated string.
    }
    *p = q + 1; // Advance the main pointer past the closing quote.
    return t;
}
/* …（同文件无关代码省略）… */
static exprtoken *jsonParseLiteralToken(const char **p, const char *end, const char *lit, int type, double num) {
    size_t l = strlen(lit);

    // Ensure we don't read past 'end'.
    if ((*p + l) > end) return NULL;

    if (strncmp(*p, lit, l) != 0) return NULL; // Literal doesn't match.

    // Check that the character *after* the literal is a valid JSON delimiter
    // (whitespace, comma, closing bracket/brace, or end of input)
    // This prevents matching "trueblabla" as "true".
    if ((*p + l) < end) {
        char next_char = *(*p + l);
        if (!i
