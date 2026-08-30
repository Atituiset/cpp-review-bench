# auto-curl-41bf4b209d

## 来源（采集溯源）
- 来源仓: curl/curl
- 源 PR: #2029 (https://github.com/curl/curl/pull/2029)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 350；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -52,13 +52,12 @@ typedef struct {
  * after call get_parm_word, str either point to string end
  * or point to any of end chars.
  */
-static char *get_param_word(char **str, char **end_pos)
+static char *get_param_word(char **str, char **end_pos, char endchar)
 {
   char *ptr = *str;
   char *word_begin = NULL;
   char *ptr2;
   char *escape = NULL;
-  const char *end_chars = ";,";
 
   /* the first non-space char is here */
   word_begin = ptr;
@@ -88,7 +87,7 @@ static char *get_param_word(char **str, char **end_pos)
           while(ptr < *end_pos);
           *end_pos = ptr2;
         }
-        while(*ptr && NULL == strchr(end_chars, *ptr))
+        while(*ptr && *ptr != ';' && *ptr != endchar)
           ++ptr;
         *str = ptr;
         return word_begin + 1;
@@ -99,7 +98,7 @@ static char *get_param_word(char **str, char **end_pos)
     ptr = word_begin;
   }
 
-  while(*ptr && NULL == strchr(end_chars, *ptr))
+  while(*ptr && *ptr != ';' && *ptr != endchar)
     ++ptr;
   *str = *end_pos = ptr;
   return word_begin;
@@ -181,9 +180,10 @@ static int read_field_headers(struct OperationConfig *config,
   /* NOTREACHED */
 }
 
-static int get_param_part(struct OperationConfig *config, char **str,
-                          char **pdata, char **ptype, char **pfilename,
-                          char **pencoder, struct curl_slist **pheaders)
+static int get_param_part(struct OperationConfig *config, char endchar,
+                          char **str, char **pdata, char **ptype,
+                          char **pfilename, char **pencoder,
+                          struct curl_slist **pheaders)
 {
   char *p = *str;
   char *type = NULL;
@@ -208,7 +208,7 @@ static int get_param_part(struct OperationConfig *config, char **str,
   while(ISSPACE(*p))
     p++;
   tp = p;
-  *pdata = get_param_word(&p, &endpos);
+  *pdata = get_param_word(&p, &endpos, endchar);
   /* If not quoted, strip trailing spaces. */
   if(*pdata == tp)
     while(endpos > *pdata && ISSPACE(endpos[-1]))
@@ -233,12 +233,10 @@ static int get_param_part(struct OperationConfig *config, char **str,
       }
 
       /* now point beyond the content-type specifier */
-      endpos = type + strlen(type_major) + strlen(type_minor) + 1;
-      for(p = endpos; ISSPACE(*p); p++)
-        ;
-      while(*p && *p != ';' && *p != ',')
-        p++;
-      endct = p;
+      p = type + strlen(type_major) + strlen(type_minor) + 1;
+      for(endct = p; *p && *p != ';' && *p != endchar; p++)
+        if(!ISSPACE(*p))
+          endct = p + 1;
       sep = *p;
     }
     else if(checkprefix("filename=", p)) {
@@ -249,7 +247,7 @@ static int get_param_part(struct OperationConfig *config, char **str,
       for(p += 9; ISSPACE(*p); p++)
         ;
       tp = p;
-      filename = get_param_word(&p, &endpos);
+      filename = get_param_word(&p, &endpos, endchar);
       /* If not quoted, strip trailing spaces. */
       if(filename == tp)
         while(endpos > filename && ISSPACE(endpos[-1]))
@@ -272,7 +270,7 @@ static int get_param_part(struct OperationConfig *config, char **str,
           p++;
         } while(ISSPACE(*p));
         tp = p;
-        hdrfile = get_param_word(&p, &endpos);
+        hdrfile = get_param_word(&p, &endpos, endchar);
         /* If not quoted, strip trailing spaces. */
         if(hdrfile == tp)
           while(endpos > hdrfile && ISSPACE(endpos[-1]))
@@ -300,7 +298,7 @@ static int get_param_part(struct OperationConfig *config, char **str,
         while(ISSPACE(*p))
           p++;
         tp = p;
-        hdr = get_param_word(&p, &endpos);
+        hdr = get_param_word(&p, &endpos, endchar);
         /* If not quoted, strip trailing spaces. */
         if(hdr == tp)
           while(endpos > hdr && ISSPACE(endpos[-1]))
@@ -322,37 +320,35 @@ static int get_param_part(struct OperationConfig *config, char **str,
       for(p += 8; ISSPACE(*p); p++)
         ;
       tp = p;
-      encoder = get_param_word(&p, &endpos);
+      encoder = g
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-curl-41bf4b209d` → 本草稿移入 `cases/defect/auto-curl-41bf4b209d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
