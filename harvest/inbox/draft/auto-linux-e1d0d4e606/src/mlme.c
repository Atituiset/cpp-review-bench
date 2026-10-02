// AUTO-DRAFT from torvalds/linux PR #d24e8ac715de2e16a53c144005b1863660a5fbea
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	bool is_6ghz = cbss->channel->band == NL80211_BAND_6GHZ;
	bool is_s1g = cbss->channel->band == NL80211_BAND_S1GHZ;
	const struct cfg80211_bss_ies *bss_ies = NULL;
	struct ieee80211_supported_band *sband;
	struct ieee802_11_elems *elems;
	u16 capab_info;
/* …（同文件无关代码省略）… */
	     (is_5ghz && link->u.mgd.conn.mode >= IEEE80211_CONN_MODE_VHT &&
	      (!elems->vht_cap_elem || !elems->vht_operation)))) {
		const struct cfg80211_bss_ies *ies;
		struct ieee802_11_elems *bss_elems;

		rcu_read_lock();
		ies = rcu_dereference(cbss->ies);
/* …（同文件无关代码省略）… */
					   "AP bug: VHT operation missing from AssocResp\n");
			}
		}
		kfree(bss_elems);
	}

	/*
/* …（同文件无关代码省略）… */
	ret = true;
out:
	kfree(elems);
	kfree(bss_ies);
	return ret;
}
