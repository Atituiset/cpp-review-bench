// AUTO-DRAFT from curl/curl PR #988
** label 'test_cleanup' is performed.
**
** Every easy_* and multi_* macros have a res_easy_* and res_multi_* macro
** counterpart that operates in tha same way with the exception that no
** jump takes place in case of failure. res_easy_* and res_multi_* macros
** should be immediately followed by checking if 'res' variable has been
** set.
