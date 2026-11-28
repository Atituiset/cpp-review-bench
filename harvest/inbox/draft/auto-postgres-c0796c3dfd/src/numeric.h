// AUTO-DRAFT from postgres/postgres PR #cf9d92d1f7550063aefbb219e1160e58ddb760ac
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

typedef struct NumericData *Numeric;
/* …（同文件无关代码省略）… */
 */
extern bool numeric_is_nan(Numeric num);
extern bool numeric_is_inf(Numeric num);
extern int32 numeric_maximum_size(int32 typmod);
extern char *numeric_out_sci(Numeric num, int scale);
extern char *numeric_normalize(Numeric num);
