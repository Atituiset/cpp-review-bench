// AUTO-DRAFT from postgres/postgres PR #50d6e533e4d9a0f70d798c534254007c83c0d428
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
List *
FetchPortalTargetList(Portal portal)
{
	/* no point in looking if we determined it doesn't return tuples */
	if (portal->strategy == PORTAL_MULTI_QUERY)
		return NIL;
	/* get the primary statement and find out what it returns */
	return FetchStatementTargetList((Node *) PortalGetPrimaryStmt(portal));
}
/* …（同文件无关代码省略）… */
List *
FetchStatementTargetList(Node *stmt)
{
	if (stmt == NULL)
		return NIL;
	if (IsA(stmt, Query))
	{
		Query	   *query = (Query *) stmt;

		if (query->commandType == CMD_UTILITY)
		{
			/* transfer attention to utility statement */
			stmt = query->utilityStmt;
		}
		else
		{
			if (query->commandType == CMD_SELECT)
				return query->targetList;
			if (query->returningList)
				return query->returningList;
			return NIL;
		}
	}
	if (IsA(stmt, PlannedStmt))
	{
		PlannedStmt *pstmt = (PlannedStmt *) stmt;

		if (pstmt->commandType == CMD_UTILITY)
		{
			/* transfer attention to utility statement */
			stmt = pstmt->utilityStmt;
		}
		else
		{
			if (pstmt->commandType == CMD_SELECT)
				return pstmt->planTree->targetlist;
			if (pstmt->hasReturning)
				return pstmt->planTree->targetlist;
			return NIL;
		}
	}
	if (IsA(stmt, FetchStmt))
	{
		FetchStmt  *fstmt = (FetchStmt *) stmt;
		Portal		subportal;

		Assert(!fstmt->ismove);
		subportal = GetPortalByName(fstmt->portalname);
		Assert(PortalIsValid(subportal));
		return FetchPortalTargetList(subportal);
	}
	if (IsA(stmt, ExecuteStmt))
	{
		ExecuteStmt *estmt = (ExecuteStmt *) stmt;
		PreparedStatement *entry;

		entry = FetchPreparedStatement(estmt->name, true);
		return FetchPreparedStatementTargetList(entry);
	}
	return NIL;
}
