// AUTO-DRAFT from torvalds/linux PR #141b164bbe186423e295b139e40e8e2d11cf1341
#define APCI1500_Z8536_CTRL_REG		0x03
/* …（同文件无关代码省略）… */
static unsigned int z8536_read(struct comedi_device *dev, unsigned int reg)
{
	unsigned long flags;
	unsigned int val;
  // <<< BUG ANCHOR
	spin_lock_irqsave(&dev->spinlock, flags);
	outb(reg, dev->iobase + APCI1500_Z8536_CTRL_REG);
	val = inb(dev->iobase + APCI1500_Z8536_CTRL_REG);
	spin_unlock_irqrestore(&dev->spinlock, flags);

	return val;
}
/* …（同文件无关代码省略）… */
			return -EINVAL;

		val = z8536_read(dev, Z8536_CT_MODE_REG(chan));
		val &= Z8536_CT_MODE_EGE;
		if (data[1] == 1)
			val |= Z8536_CT_MODE_EGE;
		else if (data[1] > 1)
