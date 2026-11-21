// AUTO-DRAFT from torvalds/linux PR #b152d7176339f11f05457799be609268e1a61cf2
  // <<< BUG ANCHOR
	for (i = hw->soc->nbase_names, j = 0; i < count_reg_names; i++, j++) {
		hw->eint->base[j] = of_iomap(np, i);
		if (IS_ERR(hw->eint->base[j])) {
			ret = PTR_ERR(hw->eint->base[j]);
			goto err_free_eint;
		}
	}
