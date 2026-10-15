// AUTO-DRAFT from redis/redis PR #15230
}

            if (top_op->opcode == EXPR_OP_OPAREN) {
                /* Open parethesis found. Our work finished. */
                exprTokenRelease(top_op);
                return 0;
            }
