// AUTO-DRAFT from torvalds/linux PR #a74306e2e676f9775457366fc047a660fbf02f26
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

struct component_clk {
	int num_parents;
	struct clk_parent_data *parent_data;
	struct device_node *node;
	int type;
	struct clk_hw *hw;
/* …（同文件无关代码省略）… */
struct clk_hw_omap_comp {
	struct clk_hw hw;
	struct device_node *comp_nodes[CLK_COMPONENT_TYPE_MAX];
	struct component_clk *comp_clks[CLK_COMPONENT_TYPE_MAX];
};
/* …（同文件无关代码省略）… */
static inline struct clk_hw *_get_hw(struct clk_hw_omap_comp *clk, int idx)
{
	if (!clk)
		return NULL;

	if (!clk->comp_clks[idx])
		return NULL;

	return clk->comp_clks[idx]->hw;
}
/* …（同文件无关代码省略）… */
#define to_clk_hw_comp(_hw) container_of(_hw, struct clk_hw_omap_comp, hw)
/* …（同文件无关代码省略）… */
	struct clk_hw_omap_comp *cclk = to_clk_hw_comp(hw);
	struct component_clk *comp;
	int num_parents = 0;
	struct clk_parent_data *parent_data = NULL;
	const char *name;
	int i;
	int ret;
/* …（同文件无关代码省略）… */
			continue;
		if (comp->num_parents) {
			num_parents = comp->num_parents;
			parent_data = comp->parent_data;
			break;
		}
	}
/* …（同文件无关代码省略）… */
	}

	name = ti_dt_clk_name(node);
	clk = clk_register_composite_pdata(NULL, name,
				     parent_data, num_parents,
				     _get_hw(cclk, CLK_COMPONENT_TYPE_MUX),
				     &ti_clk_mux_ops,
				     _get_hw(cclk, CLK_COMPONENT_TYPE_DIVIDER),
/* …（同文件无关代码省略）… */
		if (!cclk->comp_clks[i])
			continue;
		list_del(&cclk->comp_clks[i]->link);
		kfree(cclk->comp_clks[i]->parent_data);
		kfree(cclk->comp_clks[i]);
	}

/* …（同文件无关代码省略）… */
				int type)
{
	unsigned int num_parents;
	struct clk_parent_data *parent_data;
	struct component_clk *clk;
	unsigned int i;

	num_parents = of_clk_get_parent_count(node);

/* …（同文件无关代码省略）… */
		return -EINVAL;
	}

	parent_data = kzalloc_objs(*parent_data, num_parents);
	if (!parent_data)
		return -ENOMEM;

	for (i = 0; i < num_parents; i++)
		parent_data[i].index = i;

	clk = kzalloc_obj(*clk);
	if (!clk) {
		kfree(parent_data);
		return -ENOMEM;
	}

	clk->num_parents = num_parents;
	clk->parent_data = parent_data;
	clk->hw = hw;
	clk->node = node;
	clk->type = type;
