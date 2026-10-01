// AUTO-DRAFT from redis/redis PR #15021
&numFields, "Parameter `numFields` should be greater than 0") != C_OK)
                return C_ERR;
  // <<< BUG ANCHOR
            args->fieldCount = (int)numFields;
            args->firstFieldPos = i + 2;

            /* Check bounds - we must have exactly the right number of fields */
            if (args->firstFieldPos + args->fieldCount > c->argc) {
                addReplyError(c, "wrong number of arguments");
                return C_ERR;
            }

            /* Skip over the field arguments */
            i = args->firstFieldPos + args->fieldCount - 1;
            continue;
