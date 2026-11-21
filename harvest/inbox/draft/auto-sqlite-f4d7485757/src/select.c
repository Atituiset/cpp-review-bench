// AUTO-DRAFT from sqlite/sqlite PR #d08676800fd24ae4230f0bcc45f0c6a014e097b4
          pEq->w.iJoin = pE2->iTable;
        }
        p->pWhere = sqlite3ExprAnd(pParse, p->pWhere, pEq);
      }
    }
