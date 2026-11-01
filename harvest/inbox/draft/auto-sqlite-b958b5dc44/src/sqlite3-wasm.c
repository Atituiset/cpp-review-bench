// AUTO-DRAFT from sqlite/sqlite PR #9e2a3083e165cf30d7d3b7d3ca4b268ac7cc3d22
**
** where T is a literal integer and P is the ptr being disposed of.
**
** T specifies the type of P: 1==sqlite3*, 2==sqlite3_stmt*
**
** TODO is T 3==sqlite3_context*.
**
** Because this happens during finalization, code must be careful
** about how it uses P. e.g. a finalizing database must not be
