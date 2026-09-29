// AUTO-DRAFT from redis/redis PR #15171
sdsfree(code);
    sdsfree(expr);
    if (lua_pcall(lua,0,1,0)) {
        ldbLog(sdscatfmt(sdsempty(),"<error> %s",lua_tostring(lua,-1)));  // <<< BUG ANCHOR
        lua_pop(lua,1);
        return;
    }
