// AUTO-DRAFT from torvalds/linux PR #394023fbe34fccec5d4db2576169d21e14c8d3d4
	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_reserve(r2.base, r2.size);
	memblock_free((void *)r1.base, r1.size);  // <<< BUG ANCHOR

	ASSERT_EQ(rgn->base, r2.base);
	ASSERT_EQ(rgn->size, r2.size);
/* …（同文件无关代码省略）… */

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r2.base, r2.size);

	ASSERT_EQ(rgn->base, r1.base);
	ASSERT_EQ(rgn->size, r1.size);
/* …（同文件无关代码省略）… */

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r2.base, r2.size);

	ASSERT_EQ(rgn->base, r2.base + r2.size);
	ASSERT_EQ(rgn->size, total_size);
/* …（同文件无关代码省略）… */
static int memblock_free_overlap_bottom_check(void)
{
	struct memblock_region *rgn;
	phys_addr_t total_size;

	rgn = &memblock.reserved.regions[0];

	struct region r1 = {
		.base = SZ_8M,
		.size = SZ_32M
	};
	struct region r2 = {
		.base = SZ_32M,
		.size = SZ_32M
	};

	PREFIX_PUSH();

	total_size = r2.base - r1.base;

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r2.base, r2.size);

	ASSERT_EQ(rgn->base, r1.base);
	ASSERT_EQ(rgn->size, total_size);

	ASSERT_EQ(memblock.reserved.cnt, 1);
	ASSERT_EQ(memblock.reserved.total_size, total_size);

	test_pass_pop();

	return 0;
}
/* …（同文件无关代码省略）… */
static int memblock_free_within_check(void)
{
	struct memblock_region *rgn1, *rgn2;
	phys_addr_t r1_size, r2_size, total_size;

	rgn1 = &memblock.reserved.regions[0];
	rgn2 = &memblock.reserved.regions[1];

	struct region r1 = {
		.base = SZ_1M,
		.size = SZ_8M
	};
	struct region r2 = {
		.base = SZ_4M,
		.size = SZ_1M
	};

	PREFIX_PUSH();

	r1_size = r2.base - r1.base;
	r2_size = (r1.base + r1.size) - (r2.base + r2.size);
	total_size = r1_size + r2_size;

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r2.base, r2.size);

	ASSERT_EQ(rgn1->base, r1.base);
	ASSERT_EQ(rgn1->size, r1_size);

	ASSERT_EQ(rgn2->base, r2.base + r2.size);
	ASSERT_EQ(rgn2->size, r2_size);

	ASSERT_EQ(memblock.reserved.cnt, 2);
	ASSERT_EQ(memblock.reserved.total_size, total_size);

	test_pass_pop();

	return 0;
}
/* …（同文件无关代码省略）… */
static int memblock_free_only_region_check(void)
{
	struct memblock_region *rgn;

	rgn = &memblock.reserved.regions[0];

	struct region r1 = {
		.base = SZ_2K,
		.size = SZ_4K
	};

	PREFIX_PUSH();

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r1.base, r1.size);

	ASSERT_EQ(rgn->base, 0);
	ASSERT_EQ(rgn->size, 0);

	ASSERT_EQ(memblock.reserved.cnt, 0);
	ASSERT_EQ(memblock.reserved.total_size, 0);

	test_pass_pop();

	return 0;
}
/* …（同文件无关代码省略）… */
 * Expect the total size of reserved memory to be updated and the counter to
 * not be updated.
 */
static int memblock_free_near_max_check(void)
{
	struct memblock_region *rgn;
	phys_addr_t total_size;
/* …（同文件无关代码省略）… */

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_free((void *)r2.base, r2.size);

	ASSERT_EQ(rgn->base, r1.base);
	ASSERT_EQ(rgn->size, total_size);
/* …（同文件无关代码省略）… */
static int memblock_free_overlap_two_check(void)
{
	struct memblock_region *rgn1, *rgn2;
	phys_addr_t new_r1_size, new_r2_size, r2_end, r3_end, total_size;

	rgn1 = &memblock.reserved.regions[0];
	rgn2 = &memblock.reserved.regions[1];

	struct region r1 = {
		.base = SZ_16M,
		.size = SZ_32M
	};
	struct region r2 = {
		.base = SZ_64M,
		.size = SZ_64M
	};
	struct region r3 = {
		.base = SZ_32M,
		.size = SZ_64M
	};

	PREFIX_PUSH();

	r2_end = r2.base + r2.size;
	r3_end = r3.base + r3.size;
	new_r1_size = r3.base - r1.base;
	new_r2_size = r2_end - r3_end;
	total_size = new_r1_size + new_r2_size;

	reset_memblock_regions();
	memblock_reserve(r1.base, r1.size);
	memblock_reserve(r2.base, r2.size);
	memblock_free((void *)r3.base, r3.size);

	ASSERT_EQ(rgn1->base, r1.base);
	ASSERT_EQ(rgn1->size, new_r1_size);

	ASSERT_EQ(rgn2->base, r3_end);
	ASSERT_EQ(rgn2->size, new_r2_size);

	ASSERT_EQ(memblock.reserved.cnt, 2);
	ASSERT_EQ(memblock.reserved.total_size, total_size);

	test_pass_pop();

	return 0;
}
/* …（同文件无关代码省略）… */
	memblock_free_overlap_bottom_check();
	memblock_free_within_check();
	memblock_free_only_region_check();
	memblock_free_near_max_check();
	memblock_free_overlap_two_check();

	prefix_pop();
