// AUTO-DRAFT from torvalds/linux PR #9628ee0c2219daaa6374b0c5f5a5c0d55e0e263b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define TCA6418_TYPE		BIT(16)
#define PCA_TYPE_MASK		GENMASK(16, 12)
/* …（同文件无关代码省略）… */
#define PCA_CHIP_TYPE(x)	((x) & PCA_TYPE_MASK)
/* …（同文件无关代码省略）… */
#define MAX_BANK 5
#define BANK_SZ 8
#define MAX_LINE	(MAX_BANK * BANK_SZ)
/* …（同文件无关代码省略）… */
#define NBANK(chip) DIV_ROUND_UP(chip->gpio_chip.ngpio, BANK_SZ)
/* …（同文件无关代码省略）… */
struct pca953x_reg_config {
	int direction;
	int output;
	int input;
	int invert;
};
/* …（同文件无关代码省略）… */
struct pca953x_chip {
	unsigned gpio_start;
	struct mutex i2c_lock;
	struct regmap *regmap;

#ifdef CONFIG_GPIO_PCA953X_IRQ
	struct mutex irq_lock;
	DECLARE_BITMAP(irq_mask, MAX_LINE);
	DECLARE_BITMAP(irq_stat, MAX_LINE);
	DECLARE_BITMAP(irq_trig_raise, MAX_LINE);
	DECLARE_BITMAP(irq_trig_fall, MAX_LINE);
	DECLARE_BITMAP(irq_trig_level_high, MAX_LINE);
	DECLARE_BITMAP(irq_trig_level_low, MAX_LINE);
#endif
	atomic_t wakeup_path;

	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	unsigned long driver_data;
	struct regulator *regulator;

	const struct pca953x_reg_config *regs;

	u8 (*recalc_addr)(struct pca953x_chip *chip, int reg, int off);
	bool (*check_reg)(struct pca953x_chip *chip, unsigned int reg,
			  u32 checkbank);
};
/* …（同文件无关代码省略）… */
static inline u8 pca953x_get_bit_mask(struct pca953x_chip *chip, unsigned int offset)
{
	unsigned int bit_pos_in_bank = offset % BANK_SZ;
	int msb = BANK_SZ - 1;

	if (PCA_CHIP_TYPE(chip->driver_data) == TCA6418_TYPE && offset <= msb)
		return BIT(msb - bit_pos_in_bank);

	return BIT(bit_pos_in_bank);
}
/* …（同文件无关代码省略）… */
static int pca953x_read_regs(struct pca953x_chip *chip, int reg, unsigned long *val)
{
	u8 regaddr = chip->recalc_addr(chip, reg, 0);
	u8 value[MAX_BANK];
	int i, ret;

	ret = regmap_bulk_read(chip->regmap, regaddr, value, NBANK(chip));
	if (ret < 0) {
		dev_err(&chip->client->dev, "failed reading register: %d\n", ret);
		return ret;
	}

	for (i = 0; i < NBANK(chip); i++)
		bitmap_set_value8(val, value[i], i * BANK_SZ);

	return 0;
}
/* …（同文件无关代码省略）… */
static int pca953x_gpio_direction_input_unlocked(struct gpio_chip *gc,
						 unsigned int off)
{
	struct pca953x_chip *chip = gpiochip_get_data(gc);
	u8 dirreg = chip->recalc_addr(chip, chip->regs->direction, off);
	u8 bit = pca953x_get_bit_mask(chip, off);

	if (PCA_CHIP_TYPE(chip->driver_data) == TCA6418_TYPE)
		return regmap_update_bits(chip->regmap, dirreg, bit, 0);

	return regmap_update_bits(chip->regmap, dirreg, bit, bit);
}
/* …（同文件无关代码省略）… */
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pca953x_chip *chip = gpiochip_get_data(gc);

	if (on)
		atomic_inc(&chip->wakeup_path);
	else
		atomic_dec(&chip->wakeup_path);

	return irq_set_irq_wake(chip->client->irq, on);
}

static void pca953x_irq_bus_lock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pca953x_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->irq_lock);
}
/* …（同文件无关代码省略）… */
	DECLARE_BITMAP(irq_mask, MAX_LINE);
	DECLARE_BITMAP(reg_direction, MAX_LINE);
	int level;

	guard(mutex)(&chip->i2c_lock);

/* …（同文件无关代码省略）… */
	}

	/* Switch direction to input if needed */
	pca953x_read_regs(chip, chip->regs->direction, reg_direction);

	bitmap_or(irq_mask, chip->irq_trig_fall, chip->irq_trig_raise, gc->ngpio);
	bitmap_or(irq_mask, irq_mask, chip->irq_trig_level_high, gc->ngpio);
/* …（同文件无关代码省略）… */
	for_each_andnot_bit(level, irq_mask, reg_direction, gc->ngpio)
		pca953x_gpio_direction_input_unlocked(&chip->gpio_chip, level);

	mutex_unlock(&chip->irq_lock);
}

/* …（同文件无关代码省略）… */
	}

	/* Remove output pins from the equation */
	pca953x_read_regs(chip, chip->regs->direction, reg_direction);

	bitmap_copy(old_stat, chip->irq_stat, gc->ngpio);

/* …（同文件无关代码省略）… */
	 * interrupt.  We have to rely on the previous read for
	 * this purpose.
	 */
	pca953x_read_regs(chip, chip->regs->direction, reg_direction);
	bitmap_and(chip->irq_stat, irq_stat, reg_direction, gc->ngpio);
	mutex_init(&chip->irq_lock);
