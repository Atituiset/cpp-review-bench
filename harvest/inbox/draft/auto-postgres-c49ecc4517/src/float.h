// AUTO-DRAFT from postgres/postgres PR #2db51901c48e44369f5e6f209a65983c206075da
extern float8 float_underflow_error_ext(struct Node *escontext);
extern float8 float_zero_divide_error_ext(struct Node *escontext);
extern int	is_infinite(float8 val);
extern float8 float8in_internal(char *num, char **endptr_p,  // <<< BUG ANCHOR
								const char *type_name, const char *orig_string,
								struct Node *escontext);
extern float4 float4in_internal(char *num, char **endptr_p,
								const char *type_name, const char *orig_string,
								struct Node *escontext);
extern char *float8out_internal(float8 num);
