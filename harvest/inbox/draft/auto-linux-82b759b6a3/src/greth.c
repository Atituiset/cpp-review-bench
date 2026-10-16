// AUTO-DRAFT from torvalds/linux PR #045958acaa75205b31578cb22c4345e28dd2e4bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define GRETH_REGLOAD(a)	    (be32_to_cpu(__raw_readl(&(a))))
#define GRETH_REGSAVE(a, v)         (__raw_writel(cpu_to_be32(v), &(a)))
/* …（同文件无关代码省略）… */
static inline int wait_for_mdio(struct greth_private *greth)
{
	unsigned long timeout = jiffies + 4*HZ/100;
	while (GRETH_REGLOAD(greth->regs->mdio) & GRETH_MII_BUSY) {
		if (time_after(jiffies, timeout))
			return 0;
	}
	return 1;
}
/* …（同文件无关代码省略）… */
static int greth_mdio_read(struct mii_bus *bus, int phy, int reg)
{
	struct greth_private *greth = bus->priv;
	int data;

	if (!wait_for_mdio(greth))
		return -EBUSY;

	GRETH_REGSAVE(greth->regs->mdio, ((phy & 0x1F) << 11) | ((reg & 0x1F) << 6) | 2);

	if (!wait_for_mdio(greth))
		return -EBUSY;

	if (!(GRETH_REGLOAD(greth->regs->mdio) & GRETH_MII_NVALID)) {
		data = (GRETH_REGLOAD(greth->regs->mdio) >> 16) & 0xFFFF;
		return data;

	} else {
		return -1;
	}
}
/* …（同文件无关代码省略）… */
static int greth_mdio_write(struct mii_bus *bus, int phy, int reg, u16 val)
{
	struct greth_private *greth = bus->priv;

	if (!wait_for_mdio(greth))
		return -EBUSY;

	GRETH_REGSAVE(greth->regs->mdio,
		      ((val & 0xFFFF) << 16) | ((phy & 0x1F) << 11) | ((reg & 0x1F) << 6) | 1);

	if (!wait_for_mdio(greth))
		return -EBUSY;

	return 0;
}
/* …（同文件无关代码省略）… */
static void greth_link_change(struct net_device *dev)
{
	struct greth_private *greth = netdev_priv(dev);
	struct phy_device *phydev = dev->phydev;
	unsigned long flags;
	int status_change = 0;
	u32 ctrl;

	spin_lock_irqsave(&greth->devlock, flags);

	if (phydev->link) {

		if ((greth->speed != phydev->speed) || (greth->duplex != phydev->duplex)) {
			ctrl = GRETH_REGLOAD(greth->regs->control) &
			       ~(GRETH_CTRL_FD | GRETH_CTRL_SP | GRETH_CTRL_GB);

			if (phydev->duplex)
				ctrl |= GRETH_CTRL_FD;

			if (phydev->speed == SPEED_100)
				ctrl |= GRETH_CTRL_SP;
			else if (phydev->speed == SPEED_1000)
				ctrl |= GRETH_CTRL_GB;

			GRETH_REGSAVE(greth->regs->control, ctrl);
			greth->speed = phydev->speed;
			greth->duplex = phydev->duplex;
			status_change = 1;
		}
	}

	if (phydev->link != greth->link) {
		if (!phydev->link) {
			greth->speed = 0;
			greth->duplex = -1;
		}
		greth->link = phydev->link;

		status_change = 1;
	}

	spin_unlock_irqrestore(&greth->devlock, flags);

	if (status_change) {
		if (phydev->link)
			pr_debug("%s: link up (%d/%s)\n",
				dev->name, phydev->speed,
				DUPLEX_FULL == phydev->duplex ? "Full" : "Half");
		else
			pr_debug("%s: link down\n", dev->name);
	}
}
/* …（同文件无关代码省略）… */
static int greth_mdio_probe(struct net_device *dev)
{
	struct greth_private *greth = netdev_priv(dev);
	struct phy_device *phy = NULL;
	int ret;

	/* Find the first PHY */
	phy = phy_find_first(greth->mdio);

	if (!phy) {
		if (netif_msg_probe(greth))
			dev_err(&dev->dev, "no PHY found\n");
		return -ENXIO;
	}

	ret = phy_connect_direct(dev, phy, &greth_link_change,
				 greth->gbit_mac ? PHY_INTERFACE_MODE_GMII : PHY_INTERFACE_MODE_MII);
	if (ret) {
		if (netif_msg_ifup(greth))
			dev_err(&dev->dev, "could not attach to PHY\n");
		return ret;
	}

	if (greth->gbit_mac)
		phy_set_max_speed(phy, SPEED_1000);
	else
		phy_set_max_speed(phy, SPEED_100);

	linkmode_copy(phy->advertising, phy->supported);

	greth->link = 0;
	greth->speed = 0;
	greth->duplex = -1;

	return 0;
}
/* …（同文件无关代码省略）… */
static int greth_mdio_init(struct greth_private *greth)
{
	int ret;
	unsigned long timeout;
	struct net_device *ndev = greth->netdev;

	greth->mdio = mdiobus_alloc();
	if (!greth->mdio) {
		return -ENOMEM;
	}

	greth->mdio->name = "greth-mdio";
	snprintf(greth->mdio->id, MII_BUS_ID_SIZE, "%s-%d", greth->mdio->name, greth->irq);
	greth->mdio->read = greth_mdio_read;
	greth->mdio->write = greth_mdio_write;
	greth->mdio->priv = greth;

	ret = mdiobus_register(greth->mdio);
	if (ret) {
		goto error;
	}

	ret = greth_mdio_probe(greth->netdev);
	if (ret) {
		if (netif_msg_probe(greth))
			dev_err(&greth->netdev->dev, "failed to probe MDIO bus\n");
		goto unreg_mdio;
	}

	phy_start(ndev->phydev);

	/* If Ethernet debug link is used make autoneg happen right away */
	if (greth->edcl && greth_edcl == 1) {
		phy_start_aneg(ndev->phydev);
		timeout = jiffies + 6*HZ;
		while (!phy_aneg_done(ndev->phydev) &&
		       time_before(jiffies, timeout)) {
		}
		phy_read_status(ndev->phydev);
		greth_link_change(greth->netdev);
	}

	return 0;

unreg_mdio:
	mdiobus_unregister(greth->mdio);
error:
	mdiobus_free(greth->mdio);
	return ret;
}
/* …（同文件无关代码省略）… */
	/* Check if MAC can handle MDIO interrupts */
	greth->mdio_int_en = (tmp >> 26) & 1;

	err = greth_mdio_init(greth);
	if (err) {
		if (netif_msg_probe(greth))
/* …（同文件无关代码省略）… */
		dev->flags &= ~IFF_MULTICAST;
	}

	dev->netdev_ops = &greth_netdev_ops;
	dev->ethtool_ops = &greth_ethtool_ops;

	err = register_netdev(dev);
