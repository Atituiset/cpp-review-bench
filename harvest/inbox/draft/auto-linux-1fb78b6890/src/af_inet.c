// AUTO-DRAFT from torvalds/linux PR #60aba79a50127e92d8c4a663d35b855e52a9da01
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
		 * AF_UNSPEC if the address is INADDR_ANY
		 */
		addr4 = (struct sockaddr_in *)addr;
		if (addr4->sin_addr.s_addr != htonl(INADDR_ANY))
			return -EAFNOSUPPORT;
		family = AF_INET;
/* …（同文件无关代码省略）… */
#define label_sk_has_perm2(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, XXXX, YYYY, CALLBACKFN) \
({								\
	int __EERROR = 0;					\
	if (label_mediates(LABEL, AA_CLASS_NET)) {		\
		struct aa_profile *PROFILE;			\
		DEFINE_AUDIT_SK(AAD, OP, CRED, SOCKSK);		\
		(AAD).subj_cred = (CRED);			\
		(AAD).request = (REQUEST);			\
		__EERROR = (XXXX);				\
		if (__EERROR == 0) {				\
			__EERROR = (YYYY);			\
			if (__EERROR == 0) {			\
				__EERROR = fn_for_each(LABEL, PROFILE,	\
						       (CALLBACKFN));	\
			}						\
		}							\
	}							\
	__EERROR;						\
})
/* …（同文件无关代码省略）… */
#define sk_has_perm2(SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, YYYYX, CALLBACKFN) \
({									\
	struct aa_label *__label;					\
	struct aa_sk_ctx *__ctx = aa_sock(SOCKSK);			\
	int __ERROR = 0;						\
	bool __needput;						\
	if (rcu_access_pointer(__ctx->label) != kernel_t) {		\
									\
		__label = begin_current_label_crit_section(&__needput);	\
                __ERROR = label_sk_has_perm2(current_cred(), __label, SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, YYYYX, CALLBACKFN); \
		end_current_label_crit_section(__label, __needput);	\
	}								\
	__ERROR;							\
})
/* …（同文件无关代码省略）… */
	if (addrlen < offsetofend(struct sockaddr, sa_family))
		return -EINVAL;
	if (addr->sa_family == AF_UNSPEC)
		return 0;

	/* do we need early bailout for !family ... */
	return sk_has_perm2(sock->sk, OP_CONNECT, AA_MAY_CONNECT, profile, ad,
