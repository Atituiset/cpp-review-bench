// AUTO-DRAFT from torvalds/linux PR #65ba8f34ec886ec72b2c07ddc8cf3d815c32869d
 * where no register reference will cause an overflow.
 */

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define arch_atomic_read(v)		READ_ONCE((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define arch_atomic_set(v,i)		WRITE_ONCE((v)->counter, (i))

#if XCHAL_HAVE_EXCLUSIVE
