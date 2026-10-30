// AUTO-DRAFT from redis/redis PR #16034
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define CLI_HELP_COMMAND 1
/* …（同文件无关代码省略）… */
typedef struct {
    int type;
    int argc;
    sds *argv;
    sds full;

    /* Only used for help on commands */
    struct commandDocs docs;
} helpEntry;
/* …（同文件无关代码省略）… */
static sds sdscat_orempty(sds params, const char *value) {
    if (value[0] == '\0') {
        return sdscat(params, "\"\"");
    }
    return sdscat(params, value);
}
/* …（同文件无关代码省略）… */
static void cliMakeCommandDocArgs(redisReply *arguments, cliCommandArg *result) {
    for (size_t j = 0; j < arguments->elements; j++) {
        cliAddCommandDocArg(&result[j], arguments->element[j]);
    }
}
/* …（同文件无关代码省略）… */
static void cliAddCommandDocArg(cliCommandArg *cmdArg, redisReply *argMap) {
    if (argMap->type != REDIS_REPLY_MAP && argMap->type != REDIS_REPLY_ARRAY) {
        return;
    }

    for (size_t i = 0; i < argMap->elements; i += 2) {
        assert(argMap->element[i]->type == REDIS_REPLY_STRING);
        char *key = argMap->element[i]->str;
        if (!strcmp(key, "name")) {
            assert(argMap->element[i + 1]->type == REDIS_REPLY_STRING);
            cmdArg->name = sdsnew(argMap->element[i + 1]->str);
        } else if (!strcmp(key, "display_text")) {
            assert(argMap->element[i + 1]->type == REDIS_REPLY_STRING);
            cmdArg->display_text = sdsnew(argMap->element[i + 1]->str);
        } else if (!strcmp(key, "token")) {
            assert(argMap->element[i + 1]->type == REDIS_REPLY_STRING);
            cmdArg->token = sdsnew(argMap->element[i + 1]->str);
        } else if (!strcmp(key, "type")) {
            assert(argMap->element[i + 1]->type == REDIS_REPLY_STRING);
            char *type = argMap->element[i + 1]->str;
            if (!strcmp(type, "string")) {
                cmdArg->type = ARG_TYPE_STRING;
            } else if (!strcmp(type, "integer")) {
                cmdArg->type = ARG_TYPE_INTEGER;
            } else if (!strcmp(type, "double")) {
                cmdArg->type = ARG_TYPE_DOUBLE;
            } else if (!strcmp(type, "key")) {
                cmdArg->type = ARG_TYPE_KEY;
            } else if (!strcmp(type, "pattern")) {
                cmdArg->type = ARG_TYPE_PATTERN;
            } else if (!strcmp(type, "unix-time")) {
                cmdArg->type = ARG_TYPE_UNIX_TIME;
            } else if (!strcmp(type, "pure-token")) {
                cmdArg->type = ARG_TYPE_PURE_TOKEN;
            } else if (!strcmp(type, "oneof")) {
                cmdArg->type = ARG_TYPE_ONEOF;
            } else if (!strcmp(type, "block")) {
                cmdArg->type = ARG_TYPE_BLOCK;
            }
        } else if (!strcmp(key, "arguments")) {
            redisReply *arguments = argMap->element[i + 1];
            cmdArg->subargs = zcalloc(arguments->elements * sizeof(cliCommandArg));
            cmdArg->numsubargs = arguments->elements;
            cliMakeCommandDocArgs(arguments, cmdArg->subargs);
        } else if (!strcmp(key, "flags")) {
            redisReply *flags = argMap->element[i + 1];
            assert(flags->type == REDIS_REPLY_SET || flags->type == REDIS_REPLY_ARRAY);
            for (size_t j = 0; j < flags->elements; j++) {
                assert(flags->element[j]->type == REDIS_REPLY_STATUS);
                char *flag = flags->element[j]->str;
                if (!strcmp(flag, "optional")) {
                    cmdArg->flags |= CMD_ARG_OPTIONAL;
                } else if (!strcmp(flag, "multiple")) {
                    cmdArg->flags |= CMD_ARG_MULTIPLE;
                } else if (!strcmp(flag, "multiple_token")) {
                    cmdArg->flags |= CMD_ARG_MULTIPLE_TOKEN;
                }
            }
        }
    }
}
/* …（同文件无关代码省略）… */
static void cliFillInCommandHelpEntry(helpEntry *help, char *cmdname, char *subcommandname) {
    help->argc = subcommandname ? 2 : 1;
    help->argv = zmalloc(sizeof(sds) * help->argc);
    help->argv[0] = sdsnew(cmdname);
    sdstoupper(help->argv[0]);
    if (subcommandname) {
        /* Subcommand name may be two words separated by a pipe character. */
        char *pipe = strchr(subcommandname, '|');
        if (pipe != NULL) {
            help->argv[1] = sdsnew(pipe + 1);
        } else {
            help->argv[1] = sdsnew(subcommandname);
        }
        sdstoupper(help->argv[1]);
    }
    sds fullname = sdsnew(help->argv[0]);
    if (subcommandname) {
        fullname = sdscat(fullname, " ");
        fullname = sdscat(fullname, help->argv[1]);
    }
    help->full = fullname;
    help->type = CLI_HELP_COMMAND;

    help->docs.name = help->full;
    help->docs.params = NULL;
    help->docs.args = NULL;
    help->docs.numargs = 0;
    help->docs.since = NULL;
}
/* …（同文件无关代码省略）… */
static helpEntry *cliInitCommandHelpEntry(char *cmdname, char *subcommandname,
                                          helpEntry *next, redisReply *specs,
                                          dict *groups) {
    helpEntry *help = next++;
    cliFillInCommandHelpEntry(help, cmdname, subcommandname);

    assert(specs->type == REDIS_REPLY_MAP || specs->type == REDIS_REPLY_ARRAY);
    for (size_t j = 0; j < specs->elements; j += 2) {
        assert(specs->element[j]->type == REDIS_REPLY_STRING);
        char *key = specs->element[j]->str;
        if (!strcmp(key, "summary")) {
/* …（同文件无关代码省略）… */
            help->docs.numargs = arguments->elements;
            cliMakeCommandDocArgs(arguments, help->docs.args);
            help->docs.params = makeHint(NULL, 0, 0, help->docs);
        } else if (!strcmp(key, "subcommands")) {
            redisReply *subcommands = specs->element[j + 1];
            assert(subcommands->type == REDIS_REPLY_MAP || subcommands->type == REDIS_REPLY_ARRAY);
            for (size_t i = 0; i < subcommands->elements; i += 2) {
                assert(subcommands->element[i]->type == REDIS_REPLY_STRING);
          
