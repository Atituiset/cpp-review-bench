// AUTO-DRAFT from sqlite/sqlite PR #5bab74a91ba9d755d8102484e871821e796df77a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
#define pf printf
/* …（同文件无关代码省略）… */
    .zDotWasm    = 0,
    .zCmppD      = 0,
    .zEmcc       = 0,
    .zEmccExtra  = "-sMEMORY64=1 -sWASM_BIGINT=1",
    .zEnv        = 0,
    .zDeps       = 0,
    .zIfCond     = 0,
/* …（同文件无关代码省略）… */
    .zDotWasm    = 0,
    .zCmppD      = "-Dtarget:es6-module",
    .zEmcc       = 0,
    .zEmccExtra  = "-sMEMORY64=1 -sWASM_BIGINT=1",
    .zEnv        = 0,
    .zDeps       = 0,
    .zIfCond     = 0,
/* …（同文件无关代码省略）… */
    .zEmcc       =
    "$(emcc.speedtest1)"
    " $(emcc.speedtest1.common)"
    " -sMEMORY64=1 -sWASM_BIGINT=1"
    " $(pre-post.speedtest164.flags)"
    " $(cflags.common)"
    " -DSQLITE_SPEEDTEST1_WASM"
/* …（同文件无关代码省略）… */
    .zDotWasm    = "sqlite3-64bit",
    .zCmppD      = "$(c-pp.D.bundler)",
    .zEmcc       = 0,
    .zEmccExtra  = "-sMEMORY64=1",
    .zEnv        = 0,
    .zDeps       = 0,
    .zIfCond     = 0,
/* …（同文件无关代码省略）… */
      pf("\t@$(call b.strip-js-emcc-bindings,$(logtag.%s))\n", zBuildName);

      pf("\t@if [ x1 = 'x$(base64.wasm)' ]; then \\\n"
         "\techo '[$(emo.magic) Embedding $(out.%s.wasm) into $@ ...]'; \\\n"
         "\telse \\\n"
         "\techo '[$(emo.megaphone) Not embedding WASM in JS. Pass base64.wasm=1 to do that.]'; \\\n"
         "\tfi\n",
         zBuildName);
      pf("\t@mv $@ $@.in; \\\n"
         "\t$(call b.c-pp.shcmd,%s,$@.in,$@,"
         " \"--delimiter=//%%\" -Dbase64.wasm=$(base64.wasm)"
