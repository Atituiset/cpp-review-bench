# auto-vim-f347faf33d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | vim/vim |
| 源 PR | [#12749](https://github.com/vim/vim/pull/12749) |
| 许可证 | Vim |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 222 |
| 编译错误数（gcc syntax-only） | 231（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #12749 (https://github.com/vim/vim/pull/12749)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1117；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 12749 Farewell to Bram and dedicate upcoming Vim 9.1 to him :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1114,7 +1114,7 @@ ex_messages(exarg_T *eap)
 	    msg_attr(
 		    // Translator: Please replace the name and email address
 		    // with the appropriate text for your translation.
-		    _("Messages maintainer: Bram Moolenaar <Bram@vim.org>"),
+		    _("Messages maintainer: The Vim Project"),
 		    HL_ATTR(HLF_T));
     }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`alloc`
- 外部函数：`an`
- 外部函数：`cell`
- 外部函数：`ch_log`
- 外部函数：`char2cells`
- 外部函数：`check_timestamps`
- 外部函数：`clip_copy_modeless_selection`
- 外部函数：`command`
- 外部函数：`correctly`
- 外部函数：`cursor_off`
- 外部函数：`disp_sb_line`
- 外部函数：`execute_redir_str`
- 外部函数：`fputs`
- 外部函数：`get_keystroke`
- 外部函数：`get_menu_index`
- 外部函数：`history`
- 外部函数：`inc_msg_scrolled`
- 外部函数：`ins_char_typebuf`
- 外部函数：`ins_typebuf`
- 外部函数：`jump_to_mouse`
- 外部函数：`main`
- 外部函数：`mch_fopen`
- 外部函数：`mch_memmove`
- 外部函数：`ml_append`
- 外部函数：`mouse_has`
- 外部函数：`msg_attr_keep`
- 外部函数：`msg_check_screen`
- 外部函数：`msg_moremsg`
- 外部函数：`msg_puts_display`
- 外部函数：`msg_puts_printf`
- 外部函数：`msg_sb_start`
- 外部函数：`msg_scroll_up`
- 外部函数：`msg_strtrunc`
- 外部函数：`out_flush`
- 外部函数：`out_str`
- 外部函数：`output`
- 外部函数：`popup_get_message_win`
- 外部函数：`popup_message_win_visible`
- 外部函数：`problem`
- 外部函数：`ptr2cells`
- 外部函数：`putc`
- 外部函数：`redraw_later`
- 外部函数：`safe_vgetc`
- 外部函数：`screen_fill`
- 外部函数：`screen_ins_lines`
- 外部函数：`screenalloc`
- 外部函数：`screenclear`
- 外部函数：`semsg`
- 外部函数：`set_must_redraw`
- 外部函数：`set_shellsize`
- 外部函数：`set_vim_var_string`
- 外部函数：`setmouse`
- 外部函数：`shell_resized`
- 外部函数：`shortmess`
- 外部函数：`starttermcap`
- 外部函数：`swapping_screen`
- 外部函数：`top`
- 外部函数：`transchar_buf`
- 外部函数：`transchar_byte_buf`
- 外部函数：`typeahead_noflush`
- 外部函数：`ui_breakcheck`
- 外部函数：`utf_head_off`
- 外部函数：`utf_iscomposing`
- 外部函数：`utf_ptr2char`
- 外部函数：`utfc_ptr2len_len`
- 外部函数：`var_redir_str`
- 外部函数：`vgetc`
- 外部函数：`vim_free`
- 外部函数：`vim_isprintc`
- 外部函数：`vim_regexec`
- 外部函数：`vim_strchr`
- 外部函数：`vim_strnsave`
- 外部函数：`vim_strsave`
- 外部函数：`vim_strsize`
- 外部函数：`write_reg_contents`
- 大写宏：`ALLOC_ONE`
- 大写宏：`CAR`
- 大写宏：`CTRL`
- 大写宏：`DBCS`
- 大写宏：`DBCS_JPNU`
- 大写宏：`ENTER`
- 大写宏：`ERROR`
- 大写宏：`ESC`
- 大写宏：`EXMODE_NORMAL`
- 大写宏：`FAIL`
- 大写宏：`FALLTHROUGH`
- 大写宏：`FALSE`
- 大写宏：`FEAT_CLIPBOARD`
- 大写宏：`FEAT_CON_DIALOG`
- 大写宏：`FEAT_EVAL`
- 大写宏：`FEAT_GUI`
- 大写宏：`FEAT_GUI_MSWIN`
- 大写宏：`FEAT_MENU`
- 大写宏：`FEAT_RIGHTLEFT`
- 大写宏：`FEAT_TERMINAL`
- 大写宏：`FILE`
- 大写宏：`HAS_MESSAGE_WINDOW`
- 大写宏：`HLF_8`
- 大写宏：`HLF_R`
- 大写宏：`HLF_T`
- 大写宏：`HL_ATTR`
- 大写宏：`IS_SPECIAL`
- 大写宏：`K_BS`
- 大写宏：`K_DOWN`
- 大写宏：`K_HOR_SCROLLBAR`
- 大写宏：`K_IGNORE`
- 大写宏：`K_LEFTDRAG`
- 大写宏：`K_LEFTMOUSE`
- 大写宏：`K_LEFTRELEASE`
- 大写宏：`K_MENU`
- 大写宏：`K_MIDDLEDRAG`
- 大写宏：`K_MIDDLEMOUSE`
- 大写宏：`K_MIDDLERELEASE`
- 大写宏：`K_MOUSEDOWN`
- 大写宏：`K_MOUSELEFT`
- 大写宏：`K_MOUSEMOVE`
- 大写宏：`K_MOUSERIGHT`
- 大写宏：`K_MOUSEUP`
- 大写宏：`K_PAGEDOWN`
- 大写宏：`K_PAGEUP`
- 大写宏：`K_RIGHTDRAG`
- 大写宏：`K_RIGHTMOUSE`
- 大写宏：`K_RIGHTRELEASE`
- 大写宏：`K_SECOND`
- 大写宏：`K_SPECIAL`
- 大写宏：`K_THIRD`
- 大写宏：`K_UP`
- 大写宏：`K_VER_SCROLLBAR`
- 大写宏：`K_X1MOUSE`
- 大写宏：`K_X2MOUSE`
- 大写宏：`MAX_MSG_HIST_LEN`
- 大写宏：`MB_MAXBYTES`
- 大写宏：`MENU_INDEX_INVALID`
- 大写宏：`MODE_ASKMORE`
- 大写宏：`MODE_CMDLINE`
- 大写宏：`MODE_HITRETURN`
- 大写宏：`MODE_SETWSIZE`
- 大写宏：`MOUSE_RETURN`
- 大写宏：`MOUSE_SETPOS`
- 大写宏：`MSG_HIST`
- 大写宏：`MSWIN`
- 大写宏：`NUL`
- 大写宏：`NULL`
- 大写宏：`SELECT_DONE`
- 大写宏：`SHM_TRUNCALL`
- 大写宏：`STRCMP`
- 大写宏：`STRLEN`
- 大写宏：`TRUE`
- 大写宏：`T_CD`
- 大写宏：`T_CE`
- 大写宏：`UNIX`
- 大写宏：`UPD_CLEAR`
- 大写宏：`UPD_VALID`
- 大写宏：`USE_ON_FLY_SCROLL`
- 大写宏：`UTF`
- 大写宏：`VIMDLL`
- 大写宏：`VIM_CLEAR`
- 大写宏：`VMS`
- 大写宏：`VV_STATUSMSG`
- 外部类型：`Add`
- 外部类型：`Adjust`
- 外部类型：`Allow`
- 外部类型：`Also`
- 外部类型：`Avoid`
- 外部类型：`Bram`
- 外部类型：`Cmdline`
- 外部类型：`Columns`
- 外部类型：`Copy`
- 外部类型：`Ctrl_C`
- 外部类型：`Ctrl_Y`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Down`
- 外部类型：`End`
- 外部类型：`Enter`
- 外部类型：`Ex`
- 外部类型：`Find`
- 外部类型：`First`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`Go`
- 外部类型：`Halfway`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Interrupt`
- 外部类型：`It`
- 外部类型：`Jump`
- 外部类型：`Last`
- 外部类型：`Make`
- 外部类型：`May`
- 外部类型：`Messages`
- 外部类型：`Moolenaar`
- 外部类型：`Needed`
- 外部类型：`Normal`
- 外部类型：`Not`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`Please`
- 外部类型：`Press`
- 外部类型：`Put`
- 外部类型：`Recording`
- 外部类型：`Remember`
- 外部类型：`Rows`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`Special`
- 外部类型：`Start`
- 外部类型：`State`
- 外部类型：`Strange`
- 外部类型：`Temporarily`
- 外部类型：`To`
- 外部类型：`Translator`
- 外部类型：`Truncate`
- 外部类型：`Up`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Visual`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Win32`
- 外部类型：`With`
- 外部类型：`Write`
- 外部类型：`msg_hist`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-vim-f347faf33d` → 本草稿移入 `cases/defect/auto-vim-f347faf33d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
