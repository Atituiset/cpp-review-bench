// AUTO-DRAFT from postgres/postgres PR #83cefb7b738c32a66a91537aa502d99891a63b11
#define EXIT_STATUS_0(st)  ((st) == 0)
/* …（同文件无关代码省略）… */
			 * during PM_STARTUP due to some dead-end child crashing: in that
			 * situation, if the startup process dies on the SIGQUIT, we need
			 * to transition to PM_WAIT_BACKENDS state which will allow
			 * PostmasterStateMachine to restart the startup process.  (On the
			 * other hand, the startup process might complete normally, if we
			 * were too late with the SIGQUIT.  In that case we'll fall
			 * through and commence normal operations.)
			 */
			if (!EXIT_STATUS_0(exitstatus))  // <<< BUG ANCHOR
			{
				if (StartupStatus == STARTUP_SIGNALED)
				{
