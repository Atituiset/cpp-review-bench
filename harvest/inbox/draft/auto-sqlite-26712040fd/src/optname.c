// AUTO-DRAFT from sqlite/sqlite PR #ad110ce72022f3da86715efab3ba09ddbc941512
#define OPT_MASK(N)   ((1ull)<<((N)-1))
/* …（同文件无关代码省略）… */
    { OPT_MASK(31),          "ExistsToJoin" },
    { OPT_MASK(32),          "UnionLimit" },
    { OPT_MASK(33),          "HavingToWhere" },
    /* Special names */
    { 0xffffffffffffffffULL, "All" },
    { 0x0000000000000000ULL, "None" },
