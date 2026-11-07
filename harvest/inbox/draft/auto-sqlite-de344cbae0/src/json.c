// AUTO-DRAFT from sqlite/sqlite PR #9f024d923873de7a58875023d31d3377dec890b7
#define JEDIT_AINS  5   /* array_insert() */

/*
** Maximum nesting depth of JSON for this implementation.
**
** This limit is needed to avoid a stack overflow in the recursive
** descent parser.  A depth of 1000 is far deeper than any sane JSON
