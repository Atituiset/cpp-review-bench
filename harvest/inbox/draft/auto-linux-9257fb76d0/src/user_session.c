// AUTO-DRAFT from torvalds/linux PR #7f75cd8ccd9588e969158c5dc92a8cc217663f5b

static const char *session_user_name(struct ksmbd_session *session)
{
	if (user_guest(session->user))
		return "(Guest)";
	else if (ksmbd_anonymous_user(session->user))
/* …（同文件无关代码省略）… */

static const char *session_account_type(struct ksmbd_session *session)
{
	if (user_guest(session->user))
		return "guest";
	if (ksmbd_anonymous_user(session->user))
		return "anonymous";
	return "user";
}
