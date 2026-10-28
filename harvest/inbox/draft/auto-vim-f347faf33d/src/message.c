// AUTO-DRAFT from vim/vim PR #12749
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
    int
msg(char *s)
{
    return msg_attr_keep(s, 0, FALSE);
}
/* …（同文件无关代码省略）… */
    int
msg_attr(char *s, int attr)
{
    return msg_attr_keep(s, attr, FALSE);
}
/* …（同文件无关代码省略）… */
    int
msg_attr_keep(
    char	*s,
    int		attr,
    int		keep)	    // TRUE: set keep_msg if it doesn't scroll
{
    static int	entered = 0;
    int		retval;
    char_u	*buf = NULL;

    // Skip messages not matching ":filter pattern".
    // Don't filter when there is an error.
    if (!emsg_on_display && message_filtered((char_u *)s))
	return TRUE;

#ifdef FEAT_EVAL
    if (attr == 0)
	set_vim_var_string(VV_STATUSMSG, (char_u *)s, -1);
#endif

    /*
     * It is possible that displaying a messages causes a problem (e.g.,
     * when redrawing the window), which causes another message, etc..	To
     * break this loop, limit the recursiveness to 3 levels.
     */
    if (entered >= 3)
	return TRUE;
    ++entered;

    // Add message to history (unless it's a repeated kept message or a
    // truncated message)
    if ((char_u *)s != keep_msg
	    || (*s != '<'
		&& last_msg_hist != NULL
		&& last_msg_hist->msg != NULL
		&& STRCMP(s, last_msg_hist->msg)))
	add_msg_hist((char_u *)s, -1, attr);

#ifdef FEAT_EVAL
    if (emsg_to_channel_log)
	// Write message in the channel log.
	ch_log(NULL, "ERROR: %s", s);
#endif

    // Truncate the message if needed.
    msg_start();
    buf = msg_strtrunc((char_u *)s, FALSE);
    if (buf != NULL)
	s = (char *)buf;

    msg_outtrans_attr((char_u *)s, attr);
    msg_clr_eos();
    retval = msg_end();

    if (keep && retval && vim_strsize((char_u *)s)
			    < (int)(Rows - cmdline_row - 1) * Columns + sc_col)
	set_keep_msg((char_u *)s, 0);

    need_fileinfo = FALSE;

    vim_free(buf);
    --entered;
    return retval;
}
/* …（同文件无关代码省略）… */
    char_u *
msg_strtrunc(
    char_u	*s,
    int		force)	    // always truncate
{
    char_u	*buf = NULL;
    int		len;
    int		room;

    // May truncate message to avoid a hit-return prompt
    if ((!msg_scroll && !need_wait_return && shortmess(SHM_TRUNCALL)
			       && !exmode_active && msg_silent == 0) || force)
    {
	len = vim_strsize(s);
	if (msg_scrolled != 0
#ifdef HAS_MESSAGE_WINDOW
		|| in_echowindow
#endif
		)
	    // Use all the columns.
	    room = (int)(Rows - msg_row) * Columns - 1;
	else
	    // Use up to 'showcmd' column.
	    room = (int)(Rows - msg_row - 1) * Columns + sc_col - 1;
	if (len > room && room > 0)
	{
	    if (enc_utf8)
		// may have up to 18 bytes per cell (6 per char, up to two
		// composing chars)
		len = (room + 2) * 18;
	    else if (enc_dbcs == DBCS_JPNU)
		// may have up to 2 bytes per cell for euc-jp
		len = (room + 2) * 2;
	    else
		len = room + 2;
	    buf = alloc(len);
	    if (buf != NULL)
		trunc_string(s, buf, room, len);
	}
    }
    return buf;
}
/* …（同文件无关代码省略）… */
    void
trunc_string(
    char_u	*s,
    char_u	*buf,
    int		room_in,
    int		buflen)
{
    size_t	room = room_in - 3; // "..." takes 3 chars
    size_t	half;
    size_t	len = 0;
    int		e;
    int		i;
    int		n;

    if (*s == NUL)
    {
	if (buflen > 0)
	    *buf = NUL;
	return;
    }

    if (room_in < 3)
	room = 0;
    half = room / 2;

    // First part: Start of the string.
    for (e = 0; len < half && e < buflen; ++e)
    {
	if (s[e] == NUL)
	{
	    // text fits without truncating!
	    buf[e] = NUL;
	    return;
	}
	n = ptr2cells(s + e);
	if (len + n > half)
	    break;
	len += n;
	buf[e] = s[e];
	if (has_mbyte)
	    for (n = (*mb_ptr2len)(s + e); --n > 0; )
	    {
		if (++e == buflen)
		    break;
		buf[e] = s[e];
	    }
    }

    // Last part: End of the string.
    i = e;
    if (enc_dbcs != 0)
    {
	// For DBCS going backwards in a string is slow, but
	// computing the cell width isn't too slow: go forward
	// until the rest fits.
	n = vim_strsize(s + i);
	while (len + n > room)
	{
	    n -= ptr2cells(s + i);
	    i += (*mb_ptr2len)(s + i);
	}
    }
    else if (enc_utf8)
    {
	// For UTF-8 we can go backwards easily.
	half = i = (int)STRLEN(s);
	for (;;)
	{
	    do
		half = half - utf_head_off(s, s + half - 1) - 1;
	    while (half > 0 && utf_iscomposing(utf_ptr2char(s + half)));
	    n = ptr2cells(s + half);
	    if (len + n > room || half == 0)
		break;
	    len += n;
	    i = (int)half;
	}
    }
    else
    {
	for (i = (int)STRLEN(s);
		   i - 1 >= 0 && len + (n = ptr2cells(s + i - 1)) <= room; --i)
	    len += n;
    }


    if (i <= e + 3)
    {
	// text fits without truncating
	if (s != buf)
	{
	    len = STRLEN(s);
	    if (len >= (size_t)buflen)
		len = buflen - 1;
	    len = len - e + 1;
	    if (len < 1)
		buf[e - 1] = NUL;
	    else
		mch_memmove(buf + e, s + e, len);
	}
    }
    else if (e + 3 < buflen)
    {
	// set the middle and copy the last part
	mch_memmove(buf + e, "...", (size_t)3);
	len = STRLEN(s + i) + 1;
	if (len >= (size_t)buflen - e - 3)
	    len = buflen - e - 3 - 1;
	mch_memmove(buf + e + 3, s + i, len);
	buf[e + 3 + len - 1] = NUL;
    }
    else
    {
	// can't fit in the "...", just truncate it
	buf[e - 1] = NUL;
    }
}
/* …（同文件无关代码省略）… */
    void
reset_last_sourcing(void)
{
    VIM_CLEAR(last_sourcing_name);
    last_sourcing_lnum = 0;
}
/* …（同文件无关代码省略）… */
    static void
add_msg_hist(
    char_u	*s,
    int		len,		// -1 for undetermined length
    int		attr)
{
    struct msg_hist *p;

    if (msg_hist_off || msg_silent != 0)
	return;

    // Don't let the message history get too big
    while (msg_hist_len > MAX_MSG_HIST_LEN)
	(void)delete_first_msg();

    // allocate an entry and add the message at the end of the history
    p = ALLOC_ONE(struct msg_hist);
    if (p == NULL)
	return;

    if (len < 0)
	len = (int)STRLEN(s);
    // remove leading and trailing newlines
    while (len > 0 && *s == '\n')
    {
	++s;
	--len;
    }
    while (len > 0 && s[len - 1] == '\n')
	--len;
    p->msg = vim_strnsave(s, len);
    p->next = NULL;
    p->attr = attr;
    if (last_msg_hist != NULL)
	last_msg_hist->next = p;
    last_msg_hist = p;
    if (first_msg_hist == NULL)
	first_msg_hist = last_msg_hist;
    ++msg_hist_len;
}
/* …（同文件无关代码省略）… */
    int
delete_first_msg(void)
{
    struct msg_hist *p;

    if (msg_hist_len <= 0)
	return FAIL;
    p = first_msg_hist;
    first_msg_hist = p->next;
    if (first_msg_hist == NULL)
	last_msg_hist = NULL;  // history is empty
    vim_free(p->msg);
    vim_free(p);
    --msg_hist_len;
    return OK;
}
/* …（同文件无关代码省略）… */
	    msg_attr(
		    // Translator: Please replace the name and email address
		    // with the appropriate text for your translation.
		    _("Messages maintainer: Bram Moolenaar <Bram@vim.org>"),
		    HL_ATTR(HLF_T));
    }

/* …（同文件无关代码省略）… */
    void
wait_return(int redraw)
{
    int		c;
    int		oldState;
    int		tmpState;
    int		had_got_int;
    int		save_reg_recording;
    FILE	*save_scriptout;

    if (redraw == TRUE)
	set_must_redraw(UPD_CLEAR);

    // If using ":silent cmd", don't wait for a return.  Also don't set
    // need_wait_return to do it later.
    if (msg_silent != 0)
	return;
#ifdef HAS_MESSAGE_WINDOW
    if (in_echowindow)
	return;
#endif

    /*
     * When inside vgetc(), we can't wait for a typed character at all.
     * With the global command (and some others) we only need one return at
     * the end. Adjust cmdline_row to avoid the next message overwriting the
     * last one.
     */
    if (vgetc_busy > 0)
	return;
    need_wait_return = TRUE;
    if (no_wait_return)
    {
	if (!exmode_active)
	    cmdline_row = msg_row;
	return;
    }

    redir_off = TRUE;		// don't redirect this message
    oldState = State;
    if (quit_more)
    {
	c = CAR;		// just pretend CR was hit
	quit_more = FALSE;
	got_int = FALSE;
    }
    else if (exmode_active)
    {
	msg_puts(" ");		// make sure the cursor is on the right line
	c = CAR;		// no need for a return in ex mode
	got_int = FALSE;
    }
    else
    {
	// Make sure the hit-return prompt is on screen when 'guioptions' was
	// just changed.
	screenalloc(FALSE);

	State = MODE_HITRETURN;
	setmouse();
#ifdef USE_ON_FLY_SCROLL
	dont_scroll = TRUE;		// disallow scrolling here
#endif
	cmdline_row = msg_row;

	// Avoid the sequence that the user types ":" at the hit-return prompt
	// to start an Ex command, but the file-changed dialog gets in the
	// way.
	if (need_check_timestamps)
	    check_timestamps(FALSE);

	hit_return_msg();

	do
	{
	    // Remember "got_int", if it is set vgetc() probably returns a
	    // CTRL-C, but we need to loop then.
	    had_got_int = got_int;

	    // Don't do mappings here, we put the character back in the
	    // typeahead buffer.
	    ++no_mapping;
	    ++allow_keys;

	    // Temporarily disable Recording. If Recording is active, the
	    // character will be recorded later, since it will be added to the
	    // typebuf after the loop
	    save_reg_recording = reg_recording;
	    save_scriptout = scriptout;
	    reg_recording = 0;
	    scriptout = NULL;
	    c = safe_vgetc();
	    if (had_got_int && !global_busy)
		got_int = FALSE;
	    --no_mapping;
	    --allow_keys;
	    reg_recording = save_reg_recording;
	    scriptout = save_scriptout;

#ifdef FEAT_CLIPBOARD
	    // Strange way to allow copying (yanking) a modeless selection at
	    // the hit-enter prompt.  Use CTRL-Y, because the same is used in
	    // Cmdline-mode and it's harmless when there is no selection.
	    if (c == Ctrl_Y && clip_star.state == SELECT_DONE)
	    {
		clip_copy_modeless_selection(TRUE);
		c = K_IGNORE;
	    }
#endif

	    /*
	     * Allow scrolling back in the messages.
	     * Also accept scroll-down commands when messages fill the screen,
	     * to avoid that typing one 'j' too many makes the messages
	     * disappear.
	     */
	    if (p_more && !p_cp)
	    {
		if (c == 'b' || c == 'k' || c == 'u' || c == 'g'
						|| c == K_UP || c == K_PAGEUP)
		{
		    if (msg_scrolled > Rows)
			// scroll back to show older messages
			do_more_prompt(c);
		    else
		    {
			msg_didout = FALSE;
			c = K_IGNORE;
			msg_col =
#ifdef FEAT_RIGHTLEFT
			    cmdmsg_rl ? Columns - 1 :
#endif
			    0;
		    }
		    if (quit_more)
		    {
			c = CAR;		// just pretend CR was hit
			quit_more = FALSE;
			got_int = FALSE;
		    }
		    else if (c != K_IGNORE)
		    {
			c = K_IGNORE;
			hit_return_msg();
		    }
		}
		else if (msg_scrolled > Rows - 2
			 && (c == 'j' || c == 'd' || c == 'f'
					   || c == K_DOWN || c == K_PAGEDOWN))
		    c = K_IGNORE;
	    }
	} while ((had_got_int && c == Ctrl_C)
				|| c == K_IGNORE
#ifdef FEAT_GUI
				|| c == K_VER_SCROLLBAR || c == K_HOR_SCROLLBAR
#endif
				|| c == K_LEFTDRAG   || c == K_LEFTRELEASE
				|| c == K_MIDDLEDRAG || c == K_MIDDLERELEASE
				|| c == K_RIGHTDRAG  || c == K_RIGHTRELEASE
				|| c == K_MOUSELEFT  || c == K_MOUSERIGHT
				|| c == K_MOUSEDOWN  || c == K_MOUSEUP
				|| c == K_MOUSEMOVE
				|| (!mouse_has(MOUSE_RETURN)
				    && mouse_row < msg_row
				    && (c == K_LEFTMOUSE
					|| c == K_MIDDLEMOUSE
					|| c == K_RIGHTMOUSE
					|| c == K_X1MOUSE
					|| c == K_X2MOUSE))
				);
	ui_breakcheck();

	// Avoid that the mouse-up event causes Visual mode to start.
	if (c == K_LEFTMOUSE || c == K_MIDDLEMOUSE || c == K_RIGHTMOUSE
					  || c == K_X1MOUSE || c == K_X2MOUSE)
	    (void)jump_to_mouse(MOUSE_SETPOS, NULL, 0);
	else if (vim_strchr((char_u *)"\r\n ", c) == NULL && c != Ctrl_C)
	{
	    // Put the character back in the typeahead buffer.  Don't use the
	    // stuff buffer, because lmaps wouldn't work.
	    ins_char_typebuf(vgetc_char, vgetc_mod_mask);
	    do_redraw = TRUE;	    // need a redraw even though there is
				    // typeahead
	}
    }
    redir_off = FALSE;

    /*
     * If the user hits ':', '?' or '/' we get a command line from the next
     * line.
     */
    if (c == ':' || c == '?' || c == '/')
    {
	if (!exmode_active)
	    cmdline_row = msg_row;
	skip_redraw = TRUE;	    // skip redraw once
	do_redraw = FALSE;
#ifdef FEAT_TERMINAL
	skip_term_loop = TRUE;
#endif
    }

    /*
     * If the window size changed set_shellsize() will redraw the screen.
     * Otherwise the screen is only redrawn if 'redraw' is set and no ':'
     * typed.
     */
    tmpState = State;
    State = oldState;		    // restore State before set_shellsize
    setmouse();
    msg_check();

#if defined(UNIX) || defined(VMS)
    /*
     * When switching screens, we need to output an extra newline on exit.
     */
    if (swapping_screen() && !termcap_active)
	newline_on_exit = TRUE;
#endif

    need_wait_return = FALSE;
    did_wait_return = TRUE;
    emsg_on_display = FALSE;	// can delete error message now
    lines_left = -1;		// reset lines_left at next msg_start()
    reset_last_sourcing();
    if (keep_msg != NULL && vim_strsize(keep_msg) >=
				  (Rows - cmdline_row - 1) * Columns + sc_col)
	VIM_CLEAR(keep_msg);	    // don't redisplay message, it's too long

    if (tmpState == MODE_SETWSIZE)  // got resize event while in vgetc()
    {
	starttermcap();		    // start termcap before redrawing
	shell_resized();
    }
    else if (!skip_redraw
	    && (redraw == TRUE || (msg_scrolled != 0 && redraw != -1)))
    {
	starttermcap();		    // start termcap before redrawing
	redraw_later(UPD_VALID);
    }
}
/* …（同文件无关代码省略）… */
    static void
hit_return_msg(void)
{
    int		save_p_more = p_more;

    p_more = FALSE;	// don't want to see this message when scrolling back
    if (msg_didout)	// start on a new line
	msg_putchar('\n');
    if (got_int)
	msg_puts(_("Interrupt: "));

    msg_puts_attr(_("Press ENTER or type command to continue"), HL_ATTR(HLF_R));
    if (!msg_use_printf())
	msg_clr_eos();
    p_more = save_p_more;
}
/* …（同文件无关代码省略）… */
    void
set_keep_msg(char_u *s, int attr)
{
    vim_free(keep_msg);
    if (s != NULL && msg_silent == 0)
	keep_msg = vim_strsave(s);
    else
	keep_msg = NULL;
    keep_msg_more = FALSE;
    keep_msg_attr = attr;
}
/* …（同文件无关代码省略）… */
    void
msg_start(void)
{
    int		did_return = FALSE;

    if (!msg_silent)
    {
	VIM_CLEAR(keep_msg);
	need_fileinfo = FALSE;
    }

#ifdef FEAT_EVAL
    if (need_clr_eos)
    {
	// Halfway an ":echo" command and getting an (error) message: clear
	// any text from the command.
	need_clr_eos = FALSE;
	msg_clr_eos();
    }
#endif

#ifdef HAS_MESSAGE_WINDOW
    if (in_echowindow)
    {
	if (popup_message_win_visible()
		    && ((msg_col > 0 && (msg_scroll || !full_screen))
			|| in_echowindow))
	{
	    win_T *wp = popup_get_message_win();

	    // start a new line
	    curbuf = wp->w_buffer;
	    ml_append(wp->w_buffer->b_ml.ml_line_count,
					      (char_u *)"", (colnr_T)0, FALSE);
	    curbuf = curwin->w_buffer;
	}
	msg_col = 0;
    }
    else
#endif
	if (!msg_scroll && full_screen)	// overwrite last message
    {
	msg_row = cmdline_row;
	msg_col =
#ifdef FEAT_RIGHTLEFT
	    cmdmsg_rl ? Columns - 1 :
#endif
	    0;
    }
    else if (msg_didout || in_echowindow)
    {
	// start message on next line
	msg_putchar('\n');
	did_return = TRUE;
	if (exmode_active != EXMODE_NORMAL)
	    cmdline_row = msg_row;
    }
    if (!msg_didany || lines_left < 0)
	msg_starthere();
    if (msg_silent == 0)
    {
	msg_didout = FALSE;		    // no output on current line yet
	cursor_off();
    }

    // when redirecting, may need to start a new line.
    if (!did_return)
	redir_write((char_u *)"\n", -1);
}
/* …（同文件无关代码省略）… */
    void
msg_starthere(void)
{
    lines_left = cmdline_row;
    msg_didany = FALSE;
}
/* …（同文件无关代码省略）… */
    void
msg_putchar(int c)
{
    msg_putchar_attr(c, 0);
}
/* …（同文件无关代码省略）… */
    void
msg_putchar_attr(int c, int attr)
{
    char_u	buf[MB_MAXBYTES + 1];

    if (IS_SPECIAL(c))
    {
	buf[0] = K_SPECIAL;
	buf[1] = K_SECOND(c);
	buf[2] = K_THIRD(c);
	buf[3] = NUL;
    }
    else
	buf[(*mb_char2bytes)(c, buf)] = NUL;
    msg_puts_attr((char *)buf, attr);
}
/* …（同文件无关代码省略）… */
    int
msg_outtrans_attr(char_u *str, int attr)
{
    return msg_outtrans_len_attr(str, (int)STRLEN(str), attr);
}
/* …（同文件无关代码省略）… */
    int
msg_outtrans_len_attr(char_u *msgstr, int len, int attr)
{
    int		retval = 0;
    char_u	*str = msgstr;
    char_u	*plain_start = msgstr;
    char_u	*s;
    int		mb_l;
    int		c;
    int		save_got_int = got_int;

    // Only quit when got_int was set in here.
    got_int = FALSE;

    // if MSG_HIST flag set, add message to history
    if (attr & MSG_HIST)
    {
	add_msg_hist(str, len, attr);
	attr &= ~MSG_HIST;
    }

    // When drawing over the command line no need to clear it later or remove
    // the mode message.
    if (msg_row >= cmdline_row && msg_col == 0)
    {
	clear_cmdline = FALSE;
	mode_displayed = FALSE;
    }

    // If the string starts with a composing character first draw a space on
    // which the composing char can be drawn.
    if (enc_utf8 && utf_iscomposing(utf_ptr2char(msgstr)))
	msg_puts_attr(" ", attr);

    /*
     * Go over the string.  Special characters are translated and printed.
     * Normal characters are printed several at a time.
     */
    while (--len >= 0 && !got_int)
    {
	if (enc_utf8)
	    // Don't include composing chars after the end.
	    mb_l = utfc_ptr2len_len(str, len + 1);
	else if (has_mbyte)
	    mb_l = (*mb_ptr2len)(str);
	else
	    mb_l = 1;
	if (has_mbyte && mb_l > 1)
	{
	    c = (*mb_ptr2char)(str);
	    if (vim_isprintc(c))
		// printable multi-byte char: count the cells.
		retval += (*mb_ptr2cells)(str);
	    else
	    {
		// unprintable multi-byte char: print the printable chars so
		// far and the translation of the unprintable char.
		if (str > plain_start)
		    msg_puts_attr_len((char *)plain_start,
					       (int)(str - plain_start), attr);
		plain_start = str + mb_l;
		msg_puts_attr((char *)transchar_buf(NULL, c),
					    attr == 0 ? HL_ATTR(HLF_8) : attr);
		retval += char2cells(c);
	    }
	    len -= mb_l - 1;
	    str += mb_l;
	}
	else
	{
	    s = transchar_byte_buf(NULL, *str);
	    if (s[1] != NUL)
	    {
		// unprintable char: print the printable chars so far and the
		// translation of the unprintable char.
		if (str > plain_start)
		    msg_puts_attr_len((char *)plain_start,
					       (int)(str - plain_start), attr);
		plain_start = str + 1;
		msg_puts_attr((char *)s, attr == 0 ? HL_ATTR(HLF_8) : attr);
		retval += (int)STRLEN(s);
	    }
	    else
		++retval;
	    ++str;
	}
    }

    if (str > plain_start && !got_int)
	// print the printable chars at the end
	msg_puts_attr_len((char *)plain_start, (int)(str - plain_start), attr);

    got_int |= save_got_int;

    return retval;
}
/* …（同文件无关代码省略）… */
    void
msg_puts(char *s)
{
    msg_puts_attr(s, 0);
}
/* …（同文件无关代码省略）… */
    void
msg_puts_attr(char *s, int attr)
{
    msg_puts_attr_len(s, -1, attr);
}
/* …（同文件无关代码省略）… */
    static void
msg_puts_attr_len(char *str, int maxlen, int attr)
{
    /*
     * If redirection is on, also write to the redirection file.
     */
    redir_write((char_u *)str, maxlen);

    /*
     * Don't print anything when using ":silent cmd".
     */
    if (msg_silent != 0)
	return;

    // if MSG_HIST flag set, add message to history
    if ((attr & MSG_HIST) && maxlen < 0)
    {
	add_msg_hist((char_u *)str, -1, attr);
	attr &= ~MSG_HIST;
    }

    // When writing something to the screen after it has scrolled, requires a
    // wait-return prompt later.  Needed when scrolling, resetting
    // need_wait_return after some prompt, and then outputting something
    // without scrolling
    // Not needed when only using CR to move the cursor.
    if (msg_scrolled != 0 && !msg_scrolled_ign && STRCMP(str, "\r") != 0)
	need_wait_return = TRUE;
    msg_didany = TRUE;		// remember that something was outputted

    /*
     * If there is no valid screen, use fprintf so we can see error messages.
     * If termcap is not active, we may be writing in an alternate console
     * window, cursor positioning may not work correctly (window size may be
     * different, e.g. for Win32 console) or we just don't know where the
     * cursor is.
     */
    if (msg_use_printf())
	msg_puts_printf((char_u *)str, maxlen);
    else
	msg_puts_display((char_u *)str, maxlen, attr, FALSE);

    need_fileinfo = FALSE;
}
/* …（同文件无关代码省略）… */
    int
message_filtered(char_u *msg)
{
    int match;

    if (cmdmod.cmod_filter_regmatch.regprog == NULL)
	return FALSE;
    match = vim_regexec(&cmdmod.cmod_filter_regmatch, msg, (colnr_T)0);
    return cmdmod.cmod_filter_force ? match : !match;
}
/* …（同文件无关代码省略）… */
    int
msg_use_printf(void)
{
    return (!msg_check_screen()
#if defined(MSWIN) && (!defined(FEAT_GUI_MSWIN) || defined(VIMDLL))
# ifdef VIMDLL
	    || (!gui.in_use && !termcap_active)
# else
	    || !termcap_active
# endif
#endif
	    || (swapping_screen() && !termcap_active)
	       );
}
/* …（同文件无关代码省略）… */
    static int
do_more_prompt(int typed_char)
{
    static int	entered = FALSE;
    int		used_typed_char = typed_char;
    int		oldState = State;
    int		c;
#ifdef FEAT_CON_DIALOG
    int		retval = FALSE;
#endif
    int		toscroll;
    msgchunk_T	*mp_last = NULL;
    msgchunk_T	*mp;
    int		i;

    // We get called recursively when a timer callback outputs a message. In
    // that case don't show another prompt. Also when at the hit-Enter prompt
    // and nothing was typed.
    if (entered || (State == MODE_HITRETURN && typed_char == 0))
	return FALSE;
    entered = TRUE;

    if (typed_char == 'G')
    {
	// "g<": Find first line on the last page.
	mp_last = msg_sb_start(last_msgchunk);
	for (i = 0; i < Rows - 2 && mp_last != NULL
					     && mp_last->sb_prev != NULL; ++i)
	    mp_last = msg_sb_start(mp_last->sb_prev);
    }

    State = MODE_ASKMORE;
    setmouse();
    if (typed_char == NUL)
	msg_moremsg(FALSE);
    for (;;)
    {
	/*
	 * Get a typed character directly from the user.
	 */
	if (used_typed_char != NUL)
	{
	    c = used_typed_char;	// was typed at hit-enter prompt
	    used_typed_char = NUL;
	}
	else
	    c = get_keystroke();

#if defined(FEAT_MENU) && defined(FEAT_GUI)
	if (c == K_MENU)
	{
	    int idx = get_menu_index(current_menu, MODE_ASKMORE);

	    // Used a menu.  If it starts with CTRL-Y, it must
	    // be a "Copy" for the clipboard.  Otherwise
	    // assume that we end
	    if (idx == MENU_INDEX_INVALID)
		continue;
	    c = *current_menu->strings[idx];
	    if (c != NUL && current_menu->strings[idx][1] != NUL)
		ins_typebuf(current_menu->strings[idx] + 1,
				current_menu->noremap[idx], 0, TRUE,
						   current_menu->silent[idx]);
	}
#endif

	toscroll = 0;
	switch (c)
	{
	case BS:		// scroll one line back
	case K_BS:
	case 'k':
	case K_UP:
	    toscroll = -1;
	    break;

	case CAR:		// one extra line
	case NL:
	case 'j':
	case K_DOWN:
	    toscroll = 1;
	    break;

	case 'u':		// Up half a page
	    toscroll = -(Rows / 2);
	    break;

	case 'd':		// Down half a page
	    toscroll = Rows / 2;
	    break;

	case 'b':		// one page back
	case K_PAGEUP:
	    toscroll = -(Rows - 1);
	    break;

	case ' ':		// one extra page
	case 'f':
	case K_PAGEDOWN:
	case K_LEFTMOUSE:
	    toscroll = Rows - 1;
	    break;

	case 'g':		// all the way back to the start
	    toscroll = -999999;
	    break;

	case 'G':		// all the way to the end
	    toscroll = 999999;
	    lines_left = 999999;
	    break;

	case ':':		// start new command line
#ifdef FEAT_CON_DIALOG
	    if (!confirm_msg_used)
#endif
	    {
		// Since got_int is set all typeahead will be flushed, but we
		// want to keep this ':', remember that in a special way.
		typeahead_noflush(':');
#ifdef FEAT_TERMINAL
		skip_term_loop = TRUE;
#endif
		cmdline_row = Rows - 1;		// put ':' on this line
		skip_redraw = TRUE;		// skip redraw once
		need_wait_return = FALSE;	// don't wait in main()
	    }
	    // FALLTHROUGH
	case 'q':		// quit
	case Ctrl_C:
	case ESC:
#ifdef FEAT_CON_DIALOG
	    if (confirm_msg_used)
	    {
		// Jump to the choices of the dialog.
		retval = TRUE;
	    }
	    else
#endif
	    {
		got_int = TRUE;
		quit_more = TRUE;
	    }
	    // When there is some more output (wrapping line) display that
	    // without another prompt.
	    lines_left = Rows - 1;
	    break;

#ifdef FEAT_CLIPBOARD
	case Ctrl_Y:
	    // Strange way to allow copying (yanking) a modeless
	    // selection at the more prompt.  Use CTRL-Y,
	    // because the same is used in Cmdline-mode and at the
	    // hit-enter prompt.  However, scrolling one line up
	    // might be expected...
	    if (clip_star.state == SELECT_DONE)
		clip_copy_modeless_selection(TRUE);
	    continue;
#endif
	default:		// no valid response
	    msg_moremsg(TRUE);
	    continue;
	}

	if (toscroll != 0)
	{
	    if (toscroll < 0)
	    {
		// go to start of last line
		if (mp_last == NULL)
		    mp = msg_sb_start(last_msgchunk);
		else if (mp_last->sb_prev != NULL)
		    mp = msg_sb_start(mp_last->sb_prev);
		else
		    mp = NULL;

		// go to start of line at top of the screen
		for (i = 0; i < Rows - 2 && mp != NULL && mp->sb_prev != NULL;
									  ++i)
		    mp = msg_sb_start(mp->sb_prev);

		if (mp != NULL && mp->sb_prev != NULL)
		{
		    // Find line to be displayed at top.
		    for (i = 0; i > toscroll; --i)
		    {
			if (mp == NULL || mp->sb_prev == NULL)
			    break;
			mp = msg_sb_start(mp->sb_prev);
			if (mp_last == NULL)
			    mp_last = msg_sb_start(last_msgchunk);
			else
			    mp_last = msg_sb_start(mp_last->sb_prev);
		    }

		    if (toscroll == -1 && screen_ins_lines(0, 0, 1,
						     (int)Rows, 0, NULL) == OK)
		    {
			// display line at top
			(void)disp_sb_line(0, mp, FALSE);
		    }
		    else
		    {
			int did_clear = screenclear();

			// redisplay all lines
			for (i = 0; mp != NULL && i < Rows - 1; ++i)
			{
			    mp = disp_sb_line(i, mp, !did_clear);
			    ++msg_scrolled;
			}
		    }
		    toscroll = 0;
		}
	    }
	    else
	    {
		// First display any text that we scrolled back.
		while (toscroll > 0 && mp_last != NULL)
		{
		    // scroll up, display line at bottom
		    msg_scroll_up();
		    inc_msg_scrolled();
		    screen_fill((int)Rows - 2, (int)Rows - 1, 0,
						   (int)Columns, ' ', ' ', 0);
		    mp_last = disp_sb_line((int)Rows - 2, mp_last, FALSE);
		    --toscroll;
		}
	    }

	    if (toscroll <= 0)
	    {
		// displayed the requested text, more prompt again
		screen_fill((int)Rows - 1, (int)Rows, 0,
						   (int)Columns, ' ', ' ', 0);
		msg_moremsg(FALSE);
		continue;
	    }

	    // display more text, return to caller
	    lines_left = toscroll;
	}

	break;
    }

    // clear the --more-- message
    screen_fill((int)Rows - 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
    State = oldState;
    setmouse();
    if (quit_more)
    {
	msg_row = Rows - 1;
	msg_col = 0;
    }
#ifdef FEAT_RIGHTLEFT
    else if (cmdmsg_rl)
	msg_col = Columns - 1;
#endif

    entered = FALSE;
#ifdef FEAT_CON_DIALOG
    return retval;
#else
    return FALSE;
#endif
}
/* …（同文件无关代码省略）… */
    void
msg_clr_eos(void)
{
    if (msg_silent == 0)
	msg_clr_eos_force();
}
/* …（同文件无关代码省略）… */
    void
msg_clr_eos_force(void)
{
#ifdef HAS_MESSAGE_WINDOW
    if (in_echowindow)
	return;  // messages go into a popup
#endif
    if (msg_use_printf())
    {
	if (full_screen)	// only when termcap codes are valid
	{
	    if (*T_CD)
		out_str(T_CD);	// clear to end of display
	    else if (*T_CE)
		out_str(T_CE);	// clear to end of line
	}
    }
    else
    {
#ifdef FEAT_RIGHTLEFT
	if (cmdmsg_rl)
	{
	    screen_fill(msg_row, msg_row + 1, 0, msg_col + 1, ' ', ' ', 0);
	    screen_fill(msg_row + 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
	}
	else
#endif
	{
	    screen_fill(msg_row, msg_row + 1, msg_col, (int)Columns,
								 ' ', ' ', 0);
	    screen_fill(msg_row + 1, (int)Rows, 0, (int)Columns, ' ', ' ', 0);
	}
    }
}
/* …（同文件无关代码省略）… */
    int
msg_end(void)
{
    /*
     * If the string is larger than the window,
     * or the ruler option is set and we run into it,
     * we have to redraw the window.
     * Do not do this if we are abandoning the file or editing the command line.
     */
    if (!exiting && need_wait_return && !(State & MODE_CMDLINE))
    {
	wait_return(FALSE);
	return FALSE;
    }
    out_flush();
    return TRUE;
}
/* …（同文件无关代码省略）… */
    void
msg_check(void)
{
    if (msg_row == Rows - 1 && msg_col >= sc_col
#ifdef HAS_MESSAGE_WINDOW
		&& !in_echowindow
#endif
	    )
    {
	need_wait_return = TRUE;
	redraw_cmdline = TRUE;
    }
}
/* …（同文件无关代码省略）… */
    static void
redir_write(char_u *str, int maxlen)
{
    char_u	*s = str;
    static int	cur_col = 0;

    // Don't do anything for displaying prompts and the like.
    if (redir_off)
	return;

    // If 'verbosefile' is set prepare for writing in that file.
    if (*p_vfile != NUL && verbose_fd == NULL)
	verbose_open();

    if (redirecting())
    {
	// If the string doesn't start with CR or NL, go to msg_col
	if (*s != '\n' && *s != '\r')
	{
	    while (cur_col < msg_col)
	    {
#ifdef FEAT_EVAL
		if (redir_execute)
		    execute_redir_str((char_u *)" ", -1);
		else if (redir_reg)
		    write_reg_contents(redir_reg, (char_u *)" ", -1, TRUE);
		else if (redir_vname)
		    var_redir_str((char_u *)" ", -1);
		else
#endif
		    if (redir_fd != NULL)
		    fputs(" ", redir_fd);
		if (verbose_fd != NULL)
		    fputs(" ", verbose_fd);
		++cur_col;
	    }
	}

#ifdef FEAT_EVAL
	if (redir_execute)
	    execute_redir_str(s, maxlen);
	else if (redir_reg)
	    write_reg_contents(redir_reg, s, maxlen, TRUE);
	else if (redir_vname)
	    var_redir_str(s, maxlen);
#endif

	// Write and adjust the current column.
	while (*s != NUL && (maxlen < 0 || (int)(s - str) < maxlen))
	{
#ifdef FEAT_EVAL
	    if (!redir_reg && !redir_vname && !redir_execute)
#endif
		if (redir_fd != NULL)
		    putc(*s, redir_fd);
	    if (verbose_fd != NULL)
		putc(*s, verbose_fd);
	    if (*s == '\r' || *s == '\n')
		cur_col = 0;
	    else if (*s == '\t')
		cur_col += (8 - cur_col % 8);
	    else
		++cur_col;
	    ++s;
	}

	if (msg_silent != 0)	// should update msg_col
	    msg_col = cur_col;
    }
}
/* …（同文件无关代码省略）… */
    int
redirecting(void)
{
    return redir_fd != NULL || *p_vfile != NUL
#ifdef FEAT_EVAL
			  || redir_reg || redir_vname || redir_execute
#endif
				       ;
}
/* …（同文件无关代码省略）… */
    int
verbose_open(void)
{
    if (verbose_fd == NULL && !verbose_did_open)
    {
	// Only give the error message once.
	verbose_did_open = TRUE;

	verbose_fd = mch_fopen((char *)p_vfile, "a");
	if (verbose_fd == NULL)
	{
	    semsg(_(e_cant_open_file_str), p_vfile);
	    return FAIL;
	}
    }
    return OK;
}
