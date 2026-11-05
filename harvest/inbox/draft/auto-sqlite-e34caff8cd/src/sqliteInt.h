// AUTO-DRAFT from sqlite/sqlite PR #f4ebb9943ff6fe8a69f82f1acc59f409be387fbb
      unsigned bNulls :1;     /* True if explicit "NULLS FIRST/LAST" */
      unsigned bUsed :1;      /* This column used in a SF_NestedFrom subquery */
      unsigned bUsingTerm:1;  /* Term from the USING clause of a NestedFrom */
      unsigned bNoExpand: 1;  /* Term is an auxiliary in NestedFrom and should
                              ** not be expanded by "*" in parent queries */
    } fg;
