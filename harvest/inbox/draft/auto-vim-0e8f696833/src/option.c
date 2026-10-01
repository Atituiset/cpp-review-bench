// AUTO-DRAFT from vim/vim PR #13354
{
	    // Don't suggest anything if cmdline is non-empty. Vim's set-=
	    // behavior requires consecutive strings and it's usually
	    // unintuitive to users if ther try to subtract multiple flags at
	    // once.
	    return FAIL;
	}
