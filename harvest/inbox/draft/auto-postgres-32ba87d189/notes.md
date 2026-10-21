# auto-postgres-32ba87d189

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b536d5a0bd7a0e35c3168f3022183f37cb5c2058](https://github.com/postgres/postgres/commit/b536d5a0bd7a0e35c3168f3022183f37cb5c2058) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 884 |
| 编译错误数（gcc syntax-only） | 138（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b536d5a0bd7a0e35c3168f3022183f37cb5c2058 (https://github.com/postgres/postgres/commit/b536d5a0bd7a0e35c3168f3022183f37cb5c2058)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 8915；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b536d5a0bd7a0e35c3168f3022183f37cb5c2058 In deparsing, parenthesize IS DOCUMENT where it is an operand. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -8912,7 +8912,6 @@ isSimpleNode(Node *node, Node *parentNode, int prettyFlags)
 		case T_CoalesceExpr:
 		case T_MinMaxExpr:
 		case T_SQLValueFunction:
-		case T_XmlExpr:
 		case T_NextValueExpr:
 		case T_NullIfExpr:
 		case T_Aggref:
@@ -9014,6 +9013,17 @@ isSimpleNode(Node *node, Node *parentNode, int prettyFlags)
 			}
 			pg_fallthrough;
 
+		case T_XmlExpr:
+
+			/*
+			 * Function-like (name(..)), except "x IS DOCUMENT", which is a
+			 * postfix test like IS NULL.  (The IsA check is needed because
+			 * T_OpExpr falls through to here.)
+			 */
+			if (IsA(node, XmlExpr) && ((XmlExpr *) node)->op != IS_DOCUMENT)
+				return true;
+			pg_fallthrough;
+
 		case T_SubLink:
 		case T_NullTest:
 		case T_BooleanTest:
@@ -9038,13 +9048,14 @@ isSimpleNode(Node *node, Node *parentNode, int prettyFlags)
 				case T_RowExpr: /* other separators */
 				case T_CoalesceExpr:	/* own parentheses */
 				case T_MinMaxExpr:	/* own parentheses */
-				case T_XmlExpr: /* own parentheses */
 				case T_NullIfExpr:	/* other separators */
 				case T_Aggref:	/* own parentheses */
 				case T_GroupingFunc:	/* own parentheses */
 				case T_WindowFunc:	/* own parentheses */
 				case T_CaseExpr:	/* other separators */
 					return true;
+				case T_XmlExpr: /* own parentheses, except IS DOCUMENT */
+					return ((XmlExpr *) parentNode)->op != IS_DOCUMENT;
 				default:
 					return false;
 			}
@@ -9090,14 +9101,15 @@ isSimpleNode(Node *node, Node *parentNode, int prettyFlags)
 				case T_RowExpr: /* other separators */
 				case T_CoalesceExpr:	/* own parentheses */
 				case T_MinMaxExpr:	/* own parentheses */
-				case T_XmlExpr: /* own parentheses */
 				case T_NullIfExpr:	/* other separators */
 				case T_Aggref:	/* own parentheses */
 				case T_GroupingFunc:	/* own parentheses */
 				case T_WindowFunc:	/* own parentheses */
 				case T_CaseExpr:	/* other separators */
 				case T_JsonExpr:	/* own parentheses */
 					return true;
+				case T_XmlExpr: /* own parentheses, except IS DOCUMENT */
+					return ((XmlExpr *) parentNode)->op != IS_DOCUMENT;
 				default:
 					return false;
 			}
@@ -10150,6 +10162,9 @@ get_rule_expr(Node *node, deparse_context *context,
 						appendStringInfoString(buf, "XMLSERIALIZE(");
 						break;
 					case IS_DOCUMENT:
+						/* postfix test, like IS NULL: not self-delimiting */
+						if (!PRETTY_PAREN(context))
+							appendStringInfoChar(buf, '(');
 						break;
 				}
 				if (xexpr->op == IS_XMLPARSE || xexpr->op == IS_XMLSERIALIZE)
@@ -10257,7 +10272,9 @@ get_rule_expr(Node *node, deparse_context *context,
 							}
 							break;
 						case IS_DOCUMENT:
-							get_rule_expr_paren((Node *) xexpr->args, context, false, node);
+							Assert(list_length(xexpr->args) == 1);
+							get_rule_expr_paren((Node *) linitial(xexpr->args),
+												context, false, node);
 							break;
 					}
 				}
@@ -10273,7 +10290,11 @@ get_rule_expr(Node *node, deparse_context *context,
 				}
 
 				if (xexpr->op == IS_DOCUMENT)
+				{
 					appendStringInfoString(buf, " IS DOCUMENT");
+					if (!PRETTY_PAREN(context))
+						appendStringInfoChar(buf, ')');
+				}
 				else
 					appendStringInfoChar(buf, ')');
 			}
@@ -10775,10 +10796,12 @@ looks_like_function(Node *node)
 		case T_CoalesceExpr:
 		case T_MinMaxExpr:
 		case T_SQLValueFunction:
-		case T_XmlExpr:
 		case T_JsonExpr:
 			/* these are all accepted by func_expr_common_subexpr */
 			return true;
+		case T_XmlExpr:
+			/* all but IS DOCUMENT, which is a postfix test */
+			return ((XmlExpr *) node)->op != IS_DOCUMENT;
 		default:
 			break;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_to_names_hash`
- 外部函数：`aliases`
- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoChar`
- 外部函数：`appendStringInfoSpaces`
- 外部函数：`appendStringInfoString`
- 外部函数：`bms_add_member`
- 外部函数：`bms_is_member`
- 外部函数：`bms_union`
- 外部函数：`build_colinfo_names_hash`
- 外部函数：`castNode`
- 外部函数：`check_stack_depth`
- 外部函数：`coerce_to_target_type`
- 外部函数：`coercion`
- 外部函数：`columns`
- 外部函数：`destroy_colinfo_names_hash`
- 外部函数：`elog`
- 外部函数：`entries`
- 外部函数：`equal`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`expandRTE`
- 外部函数：`expandRecordVariable`
- 外部函数：`expand_colnames_array_to`
- 外部函数：`exprIsLengthCoercion`
- 外部函数：`exprType`
- 外部函数：`exprTypmod`
- 外部函数：`fail`
- 外部函数：`find_recursive_union`
- 外部函数：`first`
- 外部函数：`flatten_group_exprs`
- 外部函数：`foreach_current_index`
- 外部函数：`format_type_be`
- 外部函数：`format_type_with_typemod`
- 外部函数：`getTypeOutputInfo`
- 外部函数：`get_aggregate_argtypes`
- 外部函数：`get_attname`
- 外部函数：`get_base_element_type`
- 外部函数：`get_basic_select_query`
- 外部函数：`get_constraint_name`
- 外部函数：`get_from_clause`
- 外部函数：`get_json_table_nested_columns`
- 外部函数：`get_lock_clause_strength`
- 外部函数：`get_namespace_name_or_temp`
- 外部函数：`get_opclass_input_type`
- 外部函数：`get_rel_name`
- 外部函数：`get_returning_clause`
- 外部函数：`get_rtable_name`
- 外部函数：`get_rte_alias`
- 外部函数：`get_rte_attribute_name`
- 外部函数：`get_rule_sortgroupclause`
- 外部函数：`get_setop_query`
- 外部函数：`get_target_list`
- 外部函数：`get_tle_by_resno`
- 外部函数：`get_typ_typrelid`
- 外部函数：`get_typcollation`
- 外部函数：`get_type_category_preferred`
- 外部函数：`get_update_query_targetlist_def`
- 外部函数：`get_values_def`
- 外部函数：`get_window_frame_options`
- 外部函数：`get_with_clause`
- 外部函数：`hash_create`
- 外部函数：`hash_destroy`
- 外部函数：`hash_search`
- 外部函数：`have`
- 外部函数：`identify_join_columns`
- 外部函数：`initStringInfo`
- 外部函数：`innerPlan`
- 外部函数：`input`
- 外部函数：`isdigit`
- 外部函数：`join`
- 外部函数：`lappend`
- 外部函数：`lcons`
- 外部函数：`left_oper`
- 外部函数：`lengthof`
- 外部函数：`lfirst`
- 外部函数：`lfirst_int`
- 外部函数：`lfirst_node`
- 外部函数：`lfirst_oid`
- 外部函数：`lfourth`
- 外部函数：`linitial`
- 外部函数：`linitial_int`
- 外部函数：`linitial_node`
- 外部函数：`linitial_oid`
- 外部函数：`list_cell_number`
- 外部函数：`list_copy`
- 外部函数：`list_copy_tail`
- 外部函数：`list_delete_first`
- 外部函数：`list_free`
- 外部函数：`list_head`
- 外部函数：`list_length`
- 外部函数：`list_make1`
- 外部函数：`list_nth`
- 外部函数：`llast`
- 外部函数：`lnext`
- 外部函数：`lookup_rowtype_tupdesc`
- 外部函数：`lookup_type_cache`
- 外部函数：`lsecond`
- 外部函数：`lsecond_node`
- 外部函数：`lthird`
- 外部函数：`lthird_node`
- 外部函数：`makeString`
- 外部函数：`make_colname_unique`
- 外部函数：`map_xml_name_to_sql_identifier`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`name`
- 外部函数：`names`
- 外部函数：`needed`
- 外部函数：`nextval`
- 外部函数：`nodeTag`
- 外部函数：`only_marker`
- 外部函数：`oper`
- 外部函数：`oprid`
- 外部函数：`outerPlan`
- 外部函数：`palloc`
- 外部函数：`palloc0_array`
- 外部函数：`palloc0_object`
- 外部函数：`palloc_array`
- 外部函数：`pfree`
- 外部函数：`pg_get_indexdef_worker`
- 外部函数：`pg_mbcliplen`
- 外部函数：`plan`
- 外部函数：`proname`
- 外部函数：`pstrdup`
- 外部函数：`query`
- 外部函数：`relation_close`
- 外部函数：`relation_open`
- 外部函数：`resulttype`
- 外部函数：`returned`
- 外部函数：`right`
- 外部函数：`rightattnos`
- 外部函数：`rt_fetch`
- 外部函数：`s`
- 外部函数：`set_rtable_names`
- 外部函数：`sign`
- 外部函数：`snprintf`
- 外部函数：`something`
- 外部函数：`sprintf`
- 外部函数：`strVal`
- 外部函数：`strchr`
- 外部函数：`strcmp`
- 外部函数：`strcspn`
- 外部函数：`strip_implicit_coercions`
- 外部函数：`strlen`
- 外部函数：`that`
- 外部函数：`type_is_rowtype`
- 大写宏：`ABSENT`
- 大写宏：`AGGKIND_IS_ORDERED_SET`
- 大写宏：`ALL`
- 大写宏：`ALL_SUBLINK`
- 大写宏：`AND`
- 大写宏：`AND_EXPR`
- 大写宏：`ANY`
- 大写宏：`ANY_SUBLINK`
- 大写宏：`ARRAY`
- 大写宏：`ARRAY_SUBLINK`
- 大写宏：`ASC`
- 大写宏：`BOOLOID`
- 大写宏：`BOTH`
- 大写宏：`CASE`
- 大写宏：`CASE_TEST_EXPR`
- 大写宏：`CAST`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`CLAOID`
- 大写宏：`CMD_DELETE`
- 大写宏：`CMD_INSERT`
- 大写宏：`CMD_MERGE`
- 大写宏：`CMD_NOTHING`
- 大写宏：`CMD_SELECT`
- 大写宏：`CMD_UPDATE`
- 大写宏：`CMD_UTILITY`
- 大写宏：`COALESCE`
- 大写宏：`COERCE_EXPLICIT_CALL`
- 大写宏：`COERCE_EXPLICIT_CAST`
- 大写宏：`COERCE_IMPLICIT_CAST`
- 大写宏：`COERCE_SQL_SYNTAX`
- 大写宏：`COLLATE`
- 大写宏：`COLLATION`
- 大写宏：`COLLOID`
- 大写宏：`COLUMNS`
- 大写宏：`CONDITIONAL`
- 大写宏：`CONFLICT`
- 大写宏：`CONSTRAINT`
- 大写宏：`CONTENT`
- 大写宏：`CROSS`
- 大写宏：`CTE`
- 大写宏：`CTE_SUBLINK`
- 大写宏：`CUBE`
- 大写宏：`CURRENT`
- 大写宏：`CURRENT_CATALOG`
- 大写宏：`CURRENT_DATE`
- 大写宏：`CURRENT_ROLE`
- 大写宏：`CURRENT_SCHEMA`
- 大写宏：`CURRENT_TIME`
- 大写宏：`CURRENT_TIMESTAMP`
- 大写宏：`CURRENT_USER`
- 大写宏：`DEFAULT`
- 大写宏：`DELETE`
- 大写宏：`DESC`
- 大写宏：`DISTINCT`
- 大写宏：`DOCUMENT`
- 大写宏：`DO_AGGSPLIT_COMBINE`
- 大写宏：`DO_AGGSPLIT_SKIPFINAL`
- 大写宏：`ELSE`
- 大写宏：`EMPTY`
- 大写宏：`ENCODING`
- 大写宏：`END`
- 大写宏：`ENR`
- 大写宏：`ERRCODE_TOO_MANY_ARGUMENTS`
- 大写宏：`ERROR`
- 大写宏：`ESCAPE`
- 大写宏：`EXCEPT`
- 大写宏：`EXISTS`
- 大写宏：`EXISTS_SUBLINK`
- 大写宏：`EXPLAIN`
- 大写宏：`EXPR_SUBLINK`
- 大写宏：`EXTRACT`
- 大写宏：`FALSE`
- 大写宏：`FETCH`
- 大写宏：`FILTER`
- 大写宏：`FIRST`
- 大写宏：`FOR`
- 大写宏：`FORMAT`
- 大写宏：`FRAMEOPTION_NONDEFAULT`
- 大写宏：`FROM`
- 大写宏：`FUNCDETAIL_AGGREGATE`
- 大写宏：`FUNCDETAIL_NORMAL`
- 大写宏：`FUNCDETAIL_NOTFOUND`
- 大写宏：`FUNCDETAIL_WINDOWFUNC`
- 大写宏：`FUNCTION`
- 大写宏：`FUNC_MAX_ARGS`
- 大写宏：`F_BTRIM_BYTEA_BYTEA`
- 大写宏：`F_BTRIM_TEXT`
- 大写宏：`F_BTRIM_TEXT_TEXT`
- 大写宏：`F_EXTRACT_TEXT_DATE`
- 大写宏：`F_EXTRACT_TEXT_INTERVAL`
- 大写宏：`F_EXTRACT_TEXT_TIME`
- 大写宏：`F_EXTRACT_TEXT_TIMESTAMP`
- 大写宏：`F_EXTRACT_TEXT_TIMESTAMPTZ`
- 大写宏：`F_EXTRACT_TEXT_TIMETZ`
- 大写宏：`F_IS_NORMALIZED`
- 大写宏：`F_LTRIM_BYTEA_BYTEA`
- 大写宏：`F_LTRIM_TEXT`
- 大写宏：`F_LTRIM_TEXT_TEXT`
- 大写宏：`F_NORMALIZE`
- 大写宏：`F_OVERLAPS_TIMESTAMPTZ_INTERVAL_TIMESTAMPTZ_INTERVAL`
- 大写宏：`F_OVERLAPS_TIMESTAMPTZ_INTERVAL_TIMESTAMPTZ_TIMESTAMPTZ`
- 大写宏：`F_OVERLAPS_TIMESTAMPTZ_TIMESTAMPTZ_TIMESTAMPTZ_INTERVAL`
- 大写宏：`F_OVERLAPS_TIMESTAMPTZ_TIMESTAMPTZ_TIMESTAMPTZ_TIMESTAMPTZ`
- 大写宏：`F_OVERLAPS_TIMESTAMP_INTERVAL_TIMESTAMP_INTERVAL`
- 大写宏：`F_OVERLAPS_TIMESTAMP_INTERVAL_TIMESTAMP_TIMESTAMP`
- 大写宏：`F_OVERLAPS_TIMESTAMP_TIMESTAMP_TIMESTAMP_INTERVAL`
- 大写宏：`F_OVERLAPS_TIMESTAMP_TIMESTAMP_TIMESTAMP_TIMESTAMP`
- 大写宏：`F_OVERLAPS_TIMETZ_TIMETZ_TIMETZ_TIMETZ`
- 大写宏：`F_OVERLAPS_TIME_INTERVAL_TIME_INTERVAL`
- 大写宏：`F_OVERLAPS_TIME_INTERVAL_TIME_TIME`
- 大写宏：`F_OVERLAPS_TIME_TIME_TIME_INTERVAL`
- 大写宏：`F_OVERLAPS_TIME_TIME_TIME_TIME`
- 大写宏：`F_OVERLAY_BIT_BIT_INT4`
- 大写宏：`F_OVERLAY_BIT_BIT_INT4_INT4`
- 大写宏：`F_OVERLAY_BYTEA_BYTEA_INT4`
- 大写宏：`F_OVERLAY_BYTEA_BYTEA_INT4_INT4`
- 大写宏：`F_OVERLAY_TEXT_TEXT_INT4`
- 大写宏：`F_OVERLAY_TEXT_TEXT_INT4_INT4`
- 大写宏：`F_PG_COLLATION_FOR`
- 大写宏：`F_POSITION_BIT_BIT`
- 大写宏：`F_POSITION_BYTEA_BYTEA`
- 大写宏：`F_POSITION_TEXT_TEXT`
- 大写宏：`F_RTRIM_BYTEA_BYTEA`
- 大写宏：`F_RTRIM_TEXT`
- 大写宏：`F_RTRIM_TEXT_TEXT`
- 大写宏：`F_SUBSTRING_BIT_INT4`
- 大写宏：`F_SUBSTRING_BIT_INT4_INT4`
- 大写宏：`F_SUBSTRING_BYTEA_INT4`
- 大写宏：`F_SUBSTRING_BYTEA_INT4_INT4`
- 大写宏：`F_SUBSTRING_TEXT_INT4`
- 大写宏：`F_SUBSTRING_TEXT_INT4_INT4`
- 大写宏：`F_SUBSTRING_TEXT_TEXT_TEXT`
- 大写宏：`F_SYSTEM_USER`
- 大写宏：`F_TIMEZONE_INTERVAL_TIMESTAMP`
- 大写宏：`F_TIMEZONE_INTERVAL_TIMESTAMPTZ`
- 大写宏：`F_TIMEZONE_INTERVAL_TIMETZ`
- 大写宏：`F_TIMEZONE_TEXT_TIMESTAMP`
- 大写宏：`F_TIMEZONE_TEXT_TIMESTAMPTZ`
- 大写宏：`F_TIMEZONE_TEXT_TIMETZ`
- 大写宏：`F_TIMEZONE_TIMESTAMP`
- 大写宏：`F_TIMEZONE_TIMESTAMPTZ`
- 大写宏：`F_TIMEZONE_TIMETZ`
- 大写宏：`F_XMLEXISTS`
- 大写宏：`GETSTRUCT`
- 大写宏：`GREATEST`
- 大写宏：`GROUP`
- 大写宏：`GROUPING`
- 大写宏：`HASHCTL`
- 大写宏：`HASH_CONTEXT`
- 大写宏：`HASH_ELEM`
- 大写宏：`HASH_ENTER`
- 大写宏：`HASH_STRINGS`
- 大写宏：`HTAB`
- 大写宏：`IGNORE`
- 大写宏：`INDENT`
- 大写宏：`INDEX_VAR`
- 大写宏：`INNER`
- 大写宏：`INNER_VAR`
- 大写宏：`INSERT`
- 大写宏：`INT4`
- 大写宏：`INT4OID`
- 大写宏：`INTERSECT`
- 大写宏：`INTO`
- 大写宏：`INT_MIN`
- 大写宏：`IS_DOCUMENT`
- 大写宏：`IS_FALSE`
- 大写宏：`IS_GREATEST`
- 大写宏：`IS_LEAST`
- 大写宏：`IS_NOT_FALSE`
- 大写宏：`IS_NOT_NULL`
- 大写宏：`IS_NOT_TRUE`
- 大写宏：`IS_NOT_UNKNOWN`
- 大写宏：`IS_NULL`
- 大写宏：`IS_TRUE`
- 大写宏：`IS_UNKNOWN`
- 大写宏：`IS_XMLCONCAT`
- 大写宏：`IS_XMLELEMENT`
- 大写宏：`IS_XMLFOREST`
- 大写宏：`IS_XMLPARSE`
- 大写宏：`IS_XMLPI`
- 大写宏：`IS_XMLROOT`
- 大写宏：`IS_XMLSERIALIZE`
- 大写宏：`JOIN`
- 大写宏：`JSCTOR_JSON_ARRAY`
- 大写宏：`JSCTOR_JSON_ARRAYAGG`
- 大写宏：`JSCTOR_JSON_ARRAY_QUERY`
- 大写宏：`JSCTOR_JSON_OBJECT`
- 大写宏：`JSCTOR_JSON_OBJECTAGG`
- 大写宏：`JSCTOR_JSON_PARSE`
- 大写宏：`JSCTOR_JSON_SCALAR`
- 大写宏：`JSCTOR_JSON_SERIALIZE`
- 大写宏：`JSON`
- 大写宏：`JSONB`
- 大写宏：`JSONBOID`
- 大写宏：`JSON_ARRAY`
- 大写宏：`JSON_ARRAYAGG`
- 大写宏：`JSON_BEHAVIOR_DEFAULT`
- 大写宏：`JSON_BEHAVIOR_EMPTY_ARRAY`
- 大写宏：`JSON_BEHAVIOR_FALSE`
- 大写宏：`JSON_BEHAVIOR_NULL`
- 大写宏：`JSON_EXISTS`
- 大写宏：`JSON_EXISTS_OP`
- 大写宏：`JSON_OBJECT`
- 大写宏：`JSON_OBJECTAGG`
- 大写宏：`JSON_QUERY`
- 大写宏：`JSON_QUERY_OP`
- 大写宏：`JSON_SCALAR`
- 大写宏：`JSON_SERIALIZE`
- 大写宏：`JSON_TABLE`
- 大写宏：`JSON_VALUE`
- 大写宏：`JSON_VALUE_OP`
- 大写宏：`JSW_CONDITIONAL`
- 大写宏：`JSW_NONE`
- 大写宏：`JSW_UNCONDITIONAL`
- 大写宏：`JSW_UNSPEC`
- 大写宏：`JS_ENC_DEFAULT`
- 大写宏：`JS_ENC_UTF16`
- 大写宏：`JS_ENC_UTF32`
- 大写宏：`JS_FORMAT_DEFAULT`
- 大写宏：`JS_FORMAT_JSON`
- 大写宏：`JS_FORMAT_JSONB`
- 大写宏：`JS_TYPE_ARRAY`
- 大写宏：`JS_TYPE_OBJECT`
- 大写宏：`JS_TYPE_SCALAR`
- 大写宏：`KEEP`
- 大写宏：`KEY`
- 大写宏：`KEYS`
- 大写宏：`LAST`
- 大写宏：`LCS_NONE`
- 大写宏：`LEADING`
- 大写宏：`LEAST`
- 大写宏：`LIMIT`
- 大写宏：`LIMIT_OPTION_WITH_TIES`
- 大写宏：`LOCAL`
- 大写宏：`LOCALTIME`
- 大写宏：`LOCALTIMESTAMP`
- 大写宏：`LOCKED`
- 大写宏：`MATCHED`
- 大写宏：`MAXVALUE`
- 大写宏：`MERGE`
- 大写宏：`MERGE_ACTION`
- 大写宏：`MERGE_WHEN_MATCHED`
- 大写宏：`MERGE_WHEN_NOT_MATCHED_BY_SOURCE`
- 大写宏：`MERGE_WHEN_NOT_MATCHED_BY_TARGET`
- 大写宏：`MINVALUE`
- 大写宏：`MULTIEXPR`
- 大写宏：`MULTIEXPR_SUBLINK`
- 大写宏：`NAME`
- 大写宏：`NAMEDATALEN`
- 大写宏：`NATURAL`
- 大写宏：`NESTED`
- 大写宏：`NEW`
- 大写宏：`NIL`
- 大写宏：`NORMALIZE`
- 大写宏：`NORMALIZED`
- 大写宏：`NOT`
- 大写宏：`NOTHING`
- 大写宏：`NOTIFY`
- 大写宏：`NOT_EXPR`
- 大写宏：`NOWAIT`
- 大写宏：`NULL`
- 大写宏：`NULLIF`
- 大写宏：`NULLS`
- 大写宏：`NUMERIC`
- 大写宏：`NUMERICOID`
- 大写宏：`OBJECT`
- 大写宏：`OFFSET`
- 大写宏：`OLD`
- 大写宏：`OMIT`
- 大写宏：`ONCONFLICT_NOTHING`
- 大写宏：`ONCONFLICT_SELECT`
- 大写宏：`ONCONFLICT_UPDATE`
- 大写宏：`OPERATOR`
- 大写宏：`OPEROID`
- 大写宏：`ORDER`
- 大写宏：`ORDINALITY`
- 大写宏：`OR_EXPR`
- 大写宏：`OUTER`
- 大写宏：`OUTER_VAR`
- 大写宏：`OVER`
- 大写宏：`OVERLAPS`
- 大写宏：`OVERLAY`
- 大写宏：`OVERRIDING`
- 大写宏：`OVERRIDING_SYSTEM_VALUE`
- 大写宏：`OVERRIDING_USER_VALUE`
- 大写宏：`PARAM_EXEC`
- 大写宏：`PARAM_EXTERN`
- 大写宏：`PARSER_IGNORE_NULLS`
- 大写宏：`PARTIAL`
- 大写宏：`PARTITION`
- 大写宏：`PARTITION_RANGE_DATUM_MAXVALUE`
- 大写宏：`PARTITION_RANGE_DATUM_MINVALUE`
- 大写宏：`PARTITION_STRATEGY_HASH`
- 大写宏：`PARTITION_STRATEGY_LIST`
- 大写宏：`PARTITION_STRATEGY_RANGE`
- 大写宏：`PASSING`
- 大写宏：`PATH`
- 大写宏：`PLACING`
- 大写宏：`PLAN`
- 大写宏：`POSITION`
- 大写宏：`PRESERVE`
- 大写宏：`PROCOID`
- 大写宏：`QUOTES`
- 大写宏：`RECORD`
- 大写宏：`RECORDOID`
- 大写宏：`RELATION`
- 大写宏：`RELOID`
- 大写宏：`RENAME`
- 大写宏：`RETURN`
- 大写宏：`RETURNING`
- 大写宏：`RHS`
- 大写宏：`ROW`
- 大写宏：`ROWCOMPARE_SUBLINK`
- 大写宏：`ROWS`
- 大写宏：`RTE`
- 大写宏：`RTE_CTE`
- 大写宏：`RTE_FUNCTION`
- 大写宏：`RTE_GROUP`
- 大写宏：`RTE_JOIN`
- 大写宏：`RTE_NAMEDTUPLESTORE`
- 大写宏：`RTE_RELATION`
- 大写宏：`RTE_RESULT`
- 大写宏：`RTE_SUBQUERY`
- 大写宏：`RTE_TABLEFUNC`
- 大写宏：`RTE_VALUES`
- 大写宏：`SCALAR`
- 大写宏：`SELECT`
- 大写宏：`SESSION_USER`
- 大写宏：`SET`
- 大写宏：`SHARE`
- 大写宏：`SIMILAR`
- 大写宏：`SKIP`
- 大写宏：`SOURCE`
- 大写宏：`SQL`
- 大写宏：`SQL99`
- 大写宏：`SQL_STR_DOUBLE`
- 大写宏：`STANDALONE`
- 大写宏：`STRIP`
- 大写宏：`SUBQUERY`
- 大写宏：`SUBSTRING`
- 大写宏：`SVFOP_CURRENT_CATALOG`
- 大写宏：`SVFOP_CURRENT_DATE`
- 大写宏：`SVFOP_CURRENT_ROLE`
- 大写宏：`SVFOP_CURRENT_SCHEMA`
- 大写宏：`SVFOP_CURRENT_TIME`
- 大写宏：`SVFOP_CURRENT_TIMESTAMP`
- 大写宏：`SVFOP_CURRENT_TIMESTAMP_N`
- 大写宏：`SVFOP_CURRENT_TIME_N`
- 大写宏：`SVFOP_CURRENT_USER`
- 大写宏：`SVFOP_LOCALTIME`
- 大写宏：`SVFOP_LOCALTIMESTAMP`
- 大写宏：`SVFOP_LOCALTIMESTAMP_N`
- 大写宏：`SVFOP_LOCALTIME_N`
- 大写宏：`SVFOP_SESSION_USER`
- 大写宏：`SVFOP_USER`
- 大写宏：`SYSTEM`
- 大写宏：`SYSTEM_USER`
- 大写宏：`TARGET`
- 大写宏：`TEXTOID`
- 大写宏：`TFT_JSON_TABLE`
- 大写宏：`TFT_XMLTABLE`
- 大写宏：`THEN`
- 大写宏：`TIES`
- 大写宏：`TIME`
- 大写宏：`TODO`
- 大写宏：`TRAILING`
- 大写宏：`TRIM`
- 大写宏：`TRUE`
- 大写宏：`TYPCATEGORY_STRING`
- 大写宏：`TYPECACHE_GT_OPR`
- 大写宏：`TYPECACHE_LT_OPR`
- 大写宏：`UNCONDITIONAL`
- 大写宏：`UNION`
- 大写宏：`UNIQUE`
- 大写宏：`UNKNOWN`
- 大写宏：`UNKNOWNOID`
- 大写宏：`UNRESERVED_KEYWORD`
- 大写宏：`UPDATE`
- 大写宏：`USER`
- 大写宏：`USE_ASSERT_CHECKING`
- 大写宏：`USING`
- 大写宏：`UTF16`
- 大写宏：`UTF32`
- 大写宏：`UTF8`
- 大写宏：`VALUE`
- 大写宏：`VALUES`
- 大写宏：`VARIADIC`
- 大写宏：`VAR_RETURNING_DEFAULT`
- 大写宏：`VAR_RETURNING_NEW`
- 大写宏：`VAR_RETURNING_OLD`
- 大写宏：`VERSION`
- 大写宏：`WHEN`
- 大写宏：`WHERE`
- 大写宏：`WHITESPACE`
- 大写宏：`WINDOW`
- 大写宏：`WITH`
- 大写宏：`WITHIN`
- 大写宏：`WITHOUT`
- 大写宏：`WRAPPER`
- 大写宏：`WRONG`
- 大写宏：`XMLATTRIBUTES`
- 大写宏：`XMLCONCAT`
- 大写宏：`XMLELEMENT`
- 大写宏：`XMLEXISTS`
- 大写宏：`XMLFOREST`
- 大写宏：`XMLNAMESPACES`
- 大写宏：`XMLOPTION_DOCUMENT`
- 大写宏：`XMLPARSE`
- 大写宏：`XMLPI`
- 大写宏：`XMLROOT`
- 大写宏：`XMLSERIALIZE`
- 大写宏：`XMLTABLE`
- 大写宏：`XML_STANDALONE_NO`
- 大写宏：`XML_STANDALONE_NO_VALUE`
- 大写宏：`XML_STANDALONE_YES`
- 大写宏：`XXX`
- 大写宏：`YES`
- 大写宏：`ZONE`
- 外部类型：`AccessShareLock`
- 外部类型：`Add`
- 外部类型：`Adopt`
- 外部类型：`Advance`
- 外部类型：`Aggref`
- 外部类型：`Also`
- 外部类型：`AlternativeSubPlan`
- 外部类型：`Alternatively`
- 外部类型：`Although`
- 外部类型：`Always`
- 外部类型：`An`
- 外部类型：`And`
- 外部类型：`Any`
- 外部类型：`Append`
- 外部类型：`AppendRelInfo`
- 外部类型：`Are`
- 外部类型：`Array`
- 外部类型：`ArrayCoerceExpr`
- 外部类型：`ArrayExpr`
- 外部类型：`As`
- 外部类型：`Assert`
- 外部类型：`Asserts`
- 外部类型：`Assign`
- 外部类型：`AttrNumber`
- 外部类型：`Avoid`
- 外部类型：`Before`
- 外部类型：`Bitmapset`
- 外部类型：`BoolExpr`
- 外部类型：`BoolExprType`
- 外部类型：`BooleanTest`
- 外部类型：`Build`
- 外部类型：`But`
- 外部类型：`Calculate`
- 外部类型：`Can`
- 外部类型：`CaseExpr`
- 外部类型：`CaseTestExpr`
- 外部类型：`CaseWhen`
- 外部类型：`Chase`
- 外部类型：`Check`
- 外部类型：`CoalesceExpr`
- 外部类型：`CoerceToDomain`
- 外部类型：`CoerceToDomains`
- 外部类型：`CoerceViaIO`
- 外部类型：`CoercionForm`
- 外部类型：`CollateExpr`
- 外部类型：`CommonTableExpr`
- 外部类型：`Const`
- 外部类型：`Construct`
- 外部类型：`ConvertRowtypeExpr`
- 外部类型：`Copy`
- 外部类型：`CteScan`
- 外部类型：`Current`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`CurrentOfExpr`
- 外部类型：`Currently`
- 外部类型：`CustomScan`
- 外部类型：`Deparse`
- 外部类型：`Detect`
- 外部类型：`Determine`
- 外部类型：`DistinctExpr`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Dropped`
- 外部类型：`Due`
- 外部类型：`During`
- 外部类型：`EXPLAINing`
- 外部类型：`Each`
- 外部类型：`Else`
- 外部类型：`Ensure`
- 外部类型：`Entries`
- 外部类型：`Expr`
- 外部类型：`Extract`
- 外部类型：`Fall`
- 外部类型：`FieldSelect`
- 外部类型：`FieldStore`
- 外部类型：`FieldStores`
- 外部类型：`Find`
- 外部类型：`First`
- 外部类型：`Flat`
- 外部类型：`Float`
- 外部类型：`Follows`
- 外部类型：`For`
- 外部类型：`Force`
- 外部类型：`Forcing`
- 外部类型：`ForeignScan`
- 外部类型：`Form_pg_attribute`
- 外部类型：`Form_pg_class`
- 外部类型：`Form_pg_collation`
- 外部类型：`Form_pg_opclass`
- 外部类型：`Form_pg_operator`
- 外部类型：`Form_pg_proc`
- 外部类型：`Found`
- 外部类型：`Free`
- 外部类型：`FromExpr`
- 外部类型：`FuncDetailCode`
- 外部类型：`FuncExpr`
- 外部类型：`Functions`
- 外部类型：`Generating`
- 外部类型：`Get`
- 外部类型：`Got`
- 外部类型：`GroupingFunc`
- 外部类型：`Guard`
- 外部类型：`Handle`
- 外部类型：`Hash`
- 外部类型：`HeapTuple`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`IndexOnlyScan`
- 外部类型：`Indexes`
- 外部类型：`InferenceElem`
- 外部类型：`Infinity`
- 外部类型：`InitPlan`
- 外部类型：`Initialize`
- 外部类型：`Insert`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`InvalidOid`
- 外部类型：`Is`
- 外部类型：`It`
- 外部类型：`Join`
- 外部类型：`JoinExpr`
- 外部类型：`JsonBehavior`
- 外部类型：`JsonBehaviorType`
- 外部类型：`JsonConstructorExpr`
- 外部类型：`JsonConstructorType`
- 外部类型：`JsonExpr`
- 外部类型：`JsonFormat`
- 外部类型：`JsonIsPredicate`
- 外部类型：`JsonReturning`
- 外部类型：`JsonTablePathScan`
- 外部类型：`JsonTablePlan`
- 外部类型：`JsonTableSiblingJoin`
- 外部类型：`JsonValueExpr`
- 外部类型：`Just`
- 外部类型：`Likewise`
- 外部类型：`Link`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`LockWaitError`
- 外部类型：`LockWaitSkip`
- 外部类型：`Look`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`MergeAction`
- 外部类型：`MergeAppend`
- 外部类型：`MinMaxExpr`
- 外部类型：`ModifyTable`
- 外部类型：`NULLs`
- 外部类型：`NaN`
- 外部类型：`Name`
- 外部类型：`NameHashEntry`
- 外部类型：`NamedArgExpr`
- 外部类型：`NestLoop`
- 外部类型：`NestLoopParam`
- 外部类型：`NestLoops`
- 外部类型：`Never`
- 外部类型：`NextValueExpr`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Non`
- 外部类型：`Nope`
- 外部类型：`Normal`
- 外部类型：`Normally`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`NotifyStmt`
- 外部类型：`Now`
- 外部类型：`NullIfExpr`
- 外部类型：`NullTest`
- 外部类型：`Oid`
- 外部类型：`Okay`
- 外部类型：`OnConflictExpr`
- 外部类型：`Only`
- 外部类型：`OpExpr`
- 外部类型：`Operator`
- 外部类型：`Operators`
- 外部类型：`Ordered`
- 外部类型：`Otherwise`
- 外部类型：`Parallel`
- 外部类型：`Param`
- 外部类型：`Params`
- 外部类型：`Parenthesize`
- 外部类型：`Parenthesizing`
- 外部类型：`Parser`
- 外部类型：`PartitionBoundSpec`
- 外部类型：`PartitionRangeDatum`
- 外部类型：`Plain`
- 外部类型：`Plan`
- 外部类型：`Prefer`
- 外部类型：`Preload`
- 外部类型：`Preserving`
- 外部类型：`Print`
- 外部类型：`Push`
- 外部类型：`Put`
- 外部类型：`Qualify`
- 外部类型：`Query`
- 外部类型：`RTEs`
- 外部类型：`RangeTblEntry`
- 外部类型：`RangeTblRef`
- 外部类型：`Recurse`
- 外部类型：`RecursiveUnion`
- 外部类型：`RelabelType`
- 外部类型：`Relation`
- 外部类型：`Remaining`
- 外部类型：`Remember`
- 外部类型：`Replace`
- 外部类型：`Represent`
- 外部类型：`Restore`
- 外部类型：`Result`
- 外部类型：`ReturningExpr`
- 外部类型：`RowCompareExpr`
- 外部类型：`RowExpr`
- 外部类型：`RowExprs`
- 外部类型：`RowMarkClause`
- 外部类型：`SQLValueFunction`
- 外部类型：`Same`
- 外部类型：`Save`
- 外部类型：`ScalarArrayOpExpr`
- 外部类型：`Scan`
- 外部类型：`ScanKeywordCategories`
- 外部类型：`ScanKeywords`
- 外部类型：`See`
- 外部类型：`Select`
- 外部类型：`Set`
- 外部类型：`Show`
- 外部类型：`Similar`
- 外部类型：`Simple`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`SortGroupClause`
- 外部类型：`Special`
- 外部类型：`Start`
- 外部类型：`String`
- 外部类型：`StringInfo`
- 外部类型：`StringInfoData`
- 外部类型：`SubLink`
- 外部类型：`SubPlan`
- 外部类型：`SubPlans`
- 外部类型：`SubqueryScan`
- 外部类型：`Subroutines`
- 外部类型：`SubscriptingRef`
- 外部类型：`Subselect`
- 外部类型：`Such`
- 外部类型：`Switch`
- 外部类型：`System`
- 外部类型：`T_Aggref`
- 外部类型：`T_AlternativeSubPlan`
- 外部类型：`T_ArrayCoerceExpr`
- 外部类型：`T_ArrayExpr`
- 外部类型：`T_BoolExpr`
- 外部类型：`T_BooleanTest`
- 外部类型：`T_CaseExpr`
- 外部类型：`T_CaseTestExpr`
- 外部类型：`T_CoalesceExpr`
- 外部类型：`T_CoerceToDomain`
- 外部类型：`T_CoerceToDomainValue`
- 外部类型：`T_CoerceViaIO`
- 外部类型：`T_CollateExpr`
- 外部类型：`T_Const`
- 外部类型：`T_ConvertRowtypeExpr`
- 外部类型：`T_CurrentOfExpr`
- 外部类型：`T_DistinctExpr`
- 外部类型：`T_FieldSelect`
- 外部类型：`T_FieldStore`
- 外部类型：`T_FuncExpr`
- 外部类型：`T_GroupingFunc`
- 外部类型：`T_InferenceElem`
- 外部类型：`T_JsonConstructorExpr`
- 外部类型：`T_JsonExpr`
- 外部类型：`T_JsonIsPredicate`
- 外部类型：`T_JsonValueExpr`
- 外部类型：`T_List`
- 外部类型：`T_MergeSupportFunc`
- 外部类型：`T_MinMaxExpr`
- 外部类型：`T_NamedArgExpr`
- 外部类型：`T_NextValueExpr`
- 外部类型：`T_NullIfExpr`
- 外部类型：`T_NullTest`
- 外部类型：`T_OpExpr`
- 外部类型：`T_Param`
- 外部类型：`T_PartitionBoundSpec`
- 外部类型：`T_RelabelType`
- 外部类型：`T_ReturningExpr`
- 外部类型：`T_RowCompareExpr`
- 外部类型：`T_RowExpr`
- 外部类型：`T_SQLValueFunction`
- 外部类型：`T_ScalarArrayOpExpr`
- 外部类型：`T_SetToDefault`
- 外部类型：`T_SubLink`
- 外部类型：`T_SubPlan`
- 外部类型：`T_SubscriptingRef`
- 外部类型：`T_TableFunc`
- 外部类型：`T_Var`
- 外部类型：`T_WindowFunc`
- 外部类型：`T_XmlExpr`
- 外部类型：`TableFunc`
- 外部类型：`TargetEntry`
- 外部类型：`Tentatively`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Though`
- 外部类型：`To`
- 外部类型：`Try`
- 外部类型：`TupleDesc`
- 外部类型：`TypeCacheEntry`
- 外部类型：`Unique`
- 外部类型：`Unnamed`
- 外部类型：`Use`
- 外部类型：`Var`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Whole`
- 外部类型：`WindowAgg`
- 外部类型：`WindowClause`
- 外部类型：`WindowFunc`
- 外部类型：`Within`
- 外部类型：`WorkTableScan`
- 外部类型：`Workspace`
- 外部类型：`XmlExpr`
- 外部类型：`Yes`
- 外部类型：`is`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-32ba87d189` → 本草稿移入 `cases/defect/auto-postgres-32ba87d189/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
