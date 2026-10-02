// AUTO-DRAFT from torvalds/linux PR #a940b03cee1524c10c16e0f73ec878bbd36202a2
#include <nvhe/gfp.h>
#include <nvhe/spinlock.h>

/*
 * Holds the relevant data for maintaining the vcpu state completely at hyp.
 */
