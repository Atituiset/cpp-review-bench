// AUTO-DRAFT from postgres/postgres PR #1b0b45c5ab1aa5dc3c71cfa8e228306c2b92875c
			 * keys, we may extract key or non-key entry or even both.  In the
			 * latter case we create OR-node.  It is possible in lax mode
			 * where arrays are automatically unwrapped, or in strict mode for
			 * jpiAny items.
			 */
  // <<< BUG ANCHOR
			if (cxt->lax)
				key_entry = GIN_MAYBE;
			else if (!last)		/* root ($) */
				key_entry = GIN_FALSE;
			else if (last->type == jpiAnyArray || last->type == jpiIndexArray)
				key_entry = GIN_TRUE;
			else if (last->type == jpiAny)
