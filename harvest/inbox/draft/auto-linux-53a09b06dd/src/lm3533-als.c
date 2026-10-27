// AUTO-DRAFT from torvalds/linux PR #d857e1f3faf998f2614dcdb52a90a27bc83bd71d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/mfd/lm3533.h>


#define LM3533_ALS_RESISTOR_MIN			1
#define LM3533_ALS_RESISTOR_MAX			127
#define LM3533_ALS_CHANNEL_CURRENT_MAX		2
#define LM3533_ALS_THRESH_MAX			3
#define LM3533_ALS_ZONE_MAX			4
/* …（同文件无关代码省略）… */
#define LM3533_REG_ALS_RESISTOR_SELECT		0x30
#define LM3533_REG_ALS_CONF			0x31
#define LM3533_REG_ALS_ZONE_INFO		0x34
#define LM3533_REG_ALS_READ_ADC_RAW		0x37
#define LM3533_REG_ALS_READ_ADC_AVERAGE		0x38
#define LM3533_REG_ALS_BOUNDARY_BASE		0x50
#define LM3533_REG_ALS_TARGET_BASE		0x60
/* …（同文件无关代码省略）… */
#define LM3533_ALS_ENABLE_MASK			0x01
#define LM3533_ALS_INPUT_MODE_MASK		0x02
#define LM3533_ALS_INT_ENABLE_MASK		0x01
/* …（同文件无关代码省略）… */
#define LM3533_ALS_FLAG_INT_ENABLED		1


struct lm3533_als {
	struct lm3533 *lm3533;
	struct platform_device *pdev;

	unsigned long flags;
	int irq;

	atomic_t zone;
	struct mutex thresh_mutex;
};


/* …（同文件无关代码省略）… */
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 reg;
	u8 val;
	int ret;

	if (average)
		reg = LM3533_REG_ALS_READ_ADC_AVERAGE;
	else
		reg = LM3533_REG_ALS_READ_ADC_RAW;

	ret = lm3533_read(als->lm3533, reg, &val);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to read adc\n");
		return ret;
/* …（同文件无关代码省略）… */
static int _lm3533_als_get_zone(struct iio_dev *indio_dev, u8 *zone)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 val;
	int ret;

	ret = lm3533_read(als->lm3533, LM3533_REG_ALS_ZONE_INFO, &val);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to read zone\n");
		return ret;
/* …（同文件无关代码省略）… */
static int lm3533_als_get_zone(struct iio_dev *indio_dev, u8 *zone)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	int ret;

	if (test_bit(LM3533_ALS_FLAG_INT_ENABLED, &als->flags)) {
		*zone = atomic_read(&als->zone);
	} else {
		ret = _lm3533_als_get_zone(indio_dev, zone);
		if (ret)
			return ret;
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static inline u8 lm3533_als_get_target_reg(unsigned channel, unsigned zone)
{
	return LM3533_REG_ALS_TARGET_BASE + 5 * channel + zone;
}

static int lm3533_als_get_target(struct iio_dev *indio_dev, unsigned channel,
							unsigned zone, u8 *val)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 reg;
/* …（同文件无关代码省略）… */
		return -EINVAL;

	reg = lm3533_als_get_target_reg(channel, zone);
	ret = lm3533_read(als->lm3533, reg, val);
	if (ret)
		dev_err(&indio_dev->dev, "failed to get target current\n");

/* …（同文件无关代码省略）… */
		return -EINVAL;

	reg = lm3533_als_get_target_reg(channel, zone);
	ret = lm3533_write(als->lm3533, reg, val);
	if (ret)
		dev_err(&indio_dev->dev, "failed to set target current\n");

/* …（同文件无关代码省略）… */
static int lm3533_als_get_current(struct iio_dev *indio_dev, unsigned channel,
								int *val)
{
	u8 zone;
	u8 target;
	int ret;

	ret = lm3533_als_get_zone(indio_dev, &zone);
/* …（同文件无关代码省略）… */
static int lm3533_als_set_int_mode(struct iio_dev *indio_dev, int enable)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 mask = LM3533_ALS_INT_ENABLE_MASK;
	u8 val;
	int ret;

	if (enable)
		val = mask;
	else
		val = 0;

	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_ZONE_INFO, val, mask);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to set int mode %d\n",
								enable);
/* …（同文件无关代码省略）… */
static int lm3533_als_get_int_mode(struct iio_dev *indio_dev, int *enable)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 mask = LM3533_ALS_INT_ENABLE_MASK;
	u8 val;
	int ret;

	ret = lm3533_read(als->lm3533, LM3533_REG_ALS_ZONE_INFO, &val);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to get int mode\n");
		return ret;
	}

	*enable = !!(val & mask);

	return 0;
}
/* …（同文件无关代码省略）… */
static inline u8 lm3533_als_get_threshold_reg(unsigned nr, bool raising)
{
	u8 offset = !raising;

	return LM3533_REG_ALS_BOUNDARY_BASE + 2 * nr + offset;
}

static int lm3533_als_get_threshold(struct iio_dev *indio_dev, unsigned nr,
							bool raising, u8 *val)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 reg;
/* …（同文件无关代码省略）… */
		return -EINVAL;

	reg = lm3533_als_get_threshold_reg(nr, raising);
	ret = lm3533_read(als->lm3533, reg, val);
	if (ret)
		dev_err(&indio_dev->dev, "failed to get threshold\n");

/* …（同文件无关代码省略）… */
							bool raising, u8 val)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 val2;
	u8 reg, reg2;
	int ret;

	if (nr > LM3533_ALS_THRESH_MAX)
/* …（同文件无关代码省略）… */
	reg2 = lm3533_als_get_threshold_reg(nr, !raising);

	mutex_lock(&als->thresh_mutex);
	ret = lm3533_read(als->lm3533, reg2, &val2);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to get threshold\n");
		goto out;
/* …（同文件无关代码省略）… */
		goto out;
	}

	ret = lm3533_write(als->lm3533, reg, val);
	if (ret) {
		dev_err(&indio_dev->dev, "failed to set threshold\n");
		goto out;
/* …（同文件无关代码省略）… */
}

static int lm3533_als_get_hysteresis(struct iio_dev *indio_dev, unsigned nr,
								u8 *val)
{
	struct lm3533_als *als = iio_priv(indio_dev);
	u8 falling;
	u8 raising;
	int ret;

	if (nr > LM3533_ALS_THRESH_MAX)
/* …（同文件无关代码省略）… */
	int enable;
	int ret;

	if (als->irq) {
		ret = lm3533_als_get_int_mode(indio_dev, &enable);
		if (ret)
			return ret;
/* …（同文件无关代码省略）… */
	u8 zone;
	int ret;

	if (!als->irq)
		return -EBUSY;

	ret = kstrtoul(buf, 0, &enable);
/* …（同文件无关代码省略）… */
enum lm3533_als_attribute_type {
	LM3533_ATTR_TYPE_HYSTERESIS,
	LM3533_ATTR_TYPE_TARGET,
	LM3533_ATTR_TYPE_THRESH_FALLING,
	LM3533_ATTR_TYPE_THRESH_RAISING,
};
/* …（同文件无关代码省略）… */
struct lm3533_als_attribute {
	struct device_attribute dev_attr;
	enum lm3533_als_attribute_type type;
	u8 val1;
	u8 val2;
};
/* …（同文件无关代码省略）… */
static inline struct lm3533_als_attribute *
to_lm3533_als_attr(struct device_attribute *attr)
{
	return container_of(attr, struct lm3533_als_attribute, dev_attr);
}
/* …（同文件无关代码省略）… */
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct lm3533_als_attribute *als_attr = to_lm3533_als_attr(attr);
	u8 val;
	int ret;

	switch (als_attr->type) {
/* …（同文件无关代码省略）… */
	.attrs = lm3533_als_attributes
};

static int lm3533_als_set_input_mode(struct lm3533_als *als, bool pwm_mode)
{
	u8 mask = LM3533_ALS_INPUT_MODE_MASK;
	u8 val;
	int ret;

	if (pwm_mode)
		val = mask;	/* pwm input */
	else
		val = 0;	/* analog input */

	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_CONF, val, mask);
	if (ret) {
		dev_err(&als->pdev->dev, "failed to set input mode %d\n",
								pwm_mode);
		return ret;
	}

	return 0;
}

static int lm3533_als_set_resistor(struct lm3533_als *als, u8 val)
{
	int ret;

	if (val < LM3533_ALS_RESISTOR_MIN || val > LM3533_ALS_RESISTOR_MAX) {
		dev_err(&als->pdev->dev, "invalid resistor value\n");
		return -EINVAL;
	}

	ret = lm3533_write(als->lm3533, LM3533_REG_ALS_RESISTOR_SELECT, val);
	if (ret) {
		dev_err(&als->pdev->dev, "failed to set resistor\n");
		return ret;
	}

	return 0;
}

static int lm3533_als_setup(struct lm3533_als *als,
			    const struct lm3533_als_platform_data *pdata)
{
	int ret;

	ret = lm3533_als_set_input_mode(als, pdata->pwm_mode);
	if (ret)
		return ret;

	/* ALS input is always high impedance in PWM-mode. */
	if (!pdata->pwm_mode) {
		ret = lm3533_als_set_resistor(als, pdata->r_select);
		if (ret)
			return ret;
	}

	return 0;
}

static int lm3533_als_setup_irq(struct lm3533_als *als, void *dev)
{
	u8 mask = LM3533_ALS_INT_ENABLE_MASK;
	int ret;

	/* Make sure interrupts are disabled. */
	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_ZONE_INFO, 0, mask);
	if (ret) {
		dev_err(&als->pdev->dev, "failed to disable interrupts\n");
		return ret;
/* …（同文件无关代码省略）… */

static int lm3533_als_enable(struct lm3533_als *als)
{
	u8 mask = LM3533_ALS_ENABLE_MASK;
	int ret;

	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_CONF, mask, mask);
	if (ret)
		dev_err(&als->pdev->dev, "failed to enable ALS\n");

/* …（同文件无关代码省略）… */

static int lm3533_als_disable(struct lm3533_als *als)
{
	u8 mask = LM3533_ALS_ENABLE_MASK;
	int ret;

	ret = lm3533_update(als->lm3533, LM3533_REG_ALS_CONF, 0, mask);
	if (ret)
		dev_err(&als->pdev->dev, "failed to disable ALS\n");

/* …（同文件无关代码省略）… */

static int lm3533_als_probe(struct platform_device *pdev)
{
	const struct lm3533_als_platform_data *pdata;
	struct lm3533 *lm3533;
	struct lm3533_als *als;
	struct iio_dev *indio_dev;
/* …（同文件无关代码省略）… */
	if (!lm3533)
		return -EINVAL;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*als));
	if (!indio_dev)
		return -ENOMEM;
/* …（同文件无关代码省略）… */
	indio_dev->channels = lm3533_als_channels;
	indio_dev->num_channels = ARRAY_SIZE(lm3533_als_channels);
	indio_dev->name = dev_name(&pdev->dev);
	iio_device_set_parent(indio_dev, pdev->dev.parent);
	indio_dev->modes = INDIO_DIRECT_MODE;

	als = iio_priv(indio_dev);
	als->lm3533 = lm3533;
	als->pdev = pdev;
	als->irq = lm3533->irq;
	atomic_set(&als->zone, 0);
	mutex_init(&als->thresh_mutex);

	platform_set_drvdata(pdev, indio_dev);

	if (als->irq) {
		ret = lm3533_als_setup_irq(als, indio_dev);
		if (ret)
			return ret;
	}

	ret = lm3533_als_setup(als, pdata);
	if (ret)
		goto err_free_irq;

/* …（同文件无关代码省略）… */
err_disable:
	lm3533_als_disable(als);
err_free_irq:
	if (als->irq)
		free_irq(als->irq, indio_dev);

	return ret;
/* …（同文件无关代码省略）… */
static void lm3533_als_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct lm3533_als *als = iio_priv(indio_dev);

	lm3533_als_set_int_mode(indio_dev, false);
	iio_device_unregister(indio_dev);
	lm3533_als_disable(als);
	if (als->irq)
		free_irq(als->irq, indio_dev);
}

static struct platform_driver lm3533_als_driver = {
	.driver	= {
		.name	= "lm3533-als",
	},
	.probe		= lm3533_als_probe,
	.remove		= lm3533_als_remove,
