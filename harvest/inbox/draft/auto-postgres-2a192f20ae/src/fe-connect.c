// AUTO-DRAFT from postgres/postgres PR #af24adce91dbbc02d194163b994f1d8346831573
  // <<< BUG ANCHOR
				res = PQgetResult(conn);
				if (res && PQresultStatus(res) == PGRES_TUPLES_OK &&
					PQntuples(res) == 1)
				{
					char	   *val = PQgetvalue(res, 0, 0);

/* …（同文件无关代码省略）… */

				res = PQgetResult(conn);
				if (res && PQresultStatus(res) == PGRES_TUPLES_OK &&
					PQntuples(res) == 1)
				{
					char	   *val = PQgetvalue(res, 0, 0);
