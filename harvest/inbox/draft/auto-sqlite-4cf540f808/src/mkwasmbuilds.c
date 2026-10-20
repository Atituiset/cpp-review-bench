// AUTO-DRAFT from sqlite/sqlite PR #c01201027e319addcc7a5fc811ecc5cbea6074b2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
#define pf printf
/* …（同文件无关代码省略）… */
    .zDotWasm    = 0,
    .zCmppD      = 0,
    .zEmcc       = 0,
    .zEmccExtra  = "$(emcc.MEMORY64) -sWASM_BIGINT=1",
    .zEnv        = 0,
    .zDeps       = 0,
    .zIfCond     = 0,
/* …（同文件无关代码省略）… */
    .zDotWasm    = 0,
    .zCmppD      = "-Dtarget:es6-module",
    .zEmcc       = 0,
    .zEmccExtra  = "$(emcc.MEMORY64) -sWASM_BIGINT=1",
    .zEnv        = 0,
    .zDeps       = 0,
    .zIfCond     = 0,
/* …（同文件无关代码省略）… */
    .zEmcc       =
    "$(emcc.speedtest1)"
    " $(emcc.speedtest1.common)"
    " $(emcc.MEMORY64) -sWASM_BIGINT=1"
    " $(pre-post.speedtest164.flags)"
    " $(cflags.common)"
    " -DSQLITE_SPEEDTEST1_WASM"
/* …（同文件无关代码省略）… */
      ** Performance gains or losses are _not_ taken into account
      ** here, only wasm file size.
      */
      "--enable-bulk-memory-opt " /* required */
      "--all-features "           /* required */
      "--disable-compact-imports " /*
         https://github.com/emscripten-core/emscripten/issues/28016
      */
      "--post-emscripten "        /* Saves roughly 12kb */
      "--strip-debug "            /* We already wasm-strip, but in
                                  ** case this environment has no
/* …（同文件无关代码省略）… */
         /* althttpd will automatically try to execute wasm files
            if they have the +x bit set. Why that bit is set
            at all is a mystery. */);
      pf("\t@$(call b.call.wasm-strip,%s)\n", zBuildName);

      pf("\t@$(call b.do.wasm-opt,%s)\n", zBuildName);
      pf("\t@$(call b.strip-js-emcc-bindings,$(logtag.%s))\n", zBuildName);

      pf("\t@if [ x1 = 'x$(base64.wasm)' ]; then \\\n"
