// AUTO-DRAFT from torvalds/linux PR #045958acaa75205b31578cb22c4345e28dd2e4bf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdio.h>
  // <<< BUG ANCHOR
#define ADIN1140_PHY_CTRL_DEFAULT	0x1000
#define ADIN1140_PHY_STATUS_DEFAULT	0x082D
#define ADIN1140_PHY_ID1		0x0283
#define ADIN1140_PHY_ID2		0xBE00
/* …（同文件无关代码省略）… */
struct adin1140_priv {
	struct net_device *netdev;
	struct oa_tc6 *tc6;
	struct mii_bus *mdiobus;
	struct phy_device *phydev;
	struct delayed_work stats_work;

	/* Protects stats[] from concurrent updates in adin1140_stats_work
	 * and reads in the get_stats functions
	 */
	spinlock_t stat_lock;
	u64 stats[ADIN1140_STATS_CNT];
};
/* …（同文件无关代码省略）… */
static int adin1140_mdiobus_read(struct mii_bus *bus, int addr, int regnum)
{
	/* The ADIN1140's standard PHY C22 register map (OA TC6 0xFF00 -
	 * 0xFF1F), of which only 0xFF00 - 0xFF03 are implemented) cannot be
	 * accessed while frames are being received by the PHY. In case this
	 * happens the CONFIG0 and CONFIG2 register values will get corrupted,
	 * getting a random value. Both reads and writes cause the same
	 * behavior. This is a workaround that avoids MDIO accesses all
	 * together. Since this is a 10BASE-T1S PHY, only the loopback and
	 * reset (AN) bits in the control register (0x0) can be written.
	 * These functionalities have custom implementations in the PHY
	 * driver. C45 accesses do not cause this issue.
	 */

	switch (regnum) {
	case MII_BMCR:
		return ADIN1140_PHY_CTRL_DEFAULT;
	case MII_BMSR:
		return ADIN1140_PHY_STATUS_DEFAULT;
	case MII_PHYSID1:
		return ADIN1140_PHY_ID1;
	case MII_PHYSID2:
		return ADIN1140_PHY_ID2;
	default:
		return 0xFFFF;
	}
}
/* …（同文件无关代码省略）… */
static int adin1140_mdiobus_write(struct mii_bus *bus, int addr, int regnum,
				  u16 val)
{
	return -EIO;
}
/* …（同文件无关代码省略）… */
static int adin1140_mdio_register(struct adin1140_priv *priv,
				  struct spi_device *spidev)
{
	priv->mdiobus = devm_mdiobus_alloc(&spidev->dev);
	if (!priv->mdiobus)
		return dev_err_probe(&spidev->dev, -ENOMEM,
				     "MDIO bus alloc failed\n");

	priv->mdiobus->name = "adin1140-mdiobus";
	priv->mdiobus->priv = priv->tc6;
	priv->mdiobus->parent = &spidev->dev;
	priv->mdiobus->phy_mask = GENMASK(31, 1);
	priv->mdiobus->read = adin1140_mdiobus_read;
	priv->mdiobus->write = adin1140_mdiobus_write;
	priv->mdiobus->read_c45 = oa_tc6_mdiobus_read_c45;
	priv->mdiobus->write_c45 = oa_tc6_mdiobus_write_c45;

	snprintf(priv->mdiobus->id, MII_BUS_ID_SIZE, "adin1140-%s.%u",
		 dev_name(&spidev->dev), spi_get_chipselect(spidev, 0));

	return devm_mdiobus_register(&spidev->dev, priv->mdiobus);
}
/* …（同文件无关代码省略）… */
static void adin1140_handle_link_change(struct net_device *netdev)
{
	phy_print_status(netdev->phydev);
}
/* …（同文件无关代码省略）… */
static void adin1140_phy_remove(void *data)
{
	phy_disconnect(data);
}
/* …（同文件无关代码省略）… */
static int adin1140_phy_init(struct adin1140_priv *priv,
			     struct spi_device *spidev)
{
	int ret;

	ret = adin1140_mdio_register(priv, spidev);
	if (ret)
		return ret;

	priv->phydev = phy_find_first(priv->mdiobus);
	if (!priv->phydev)
		return dev_err_probe(&spidev->dev, -ENODEV, "No PHY found\n");

	priv->phydev->is_internal = true;
	ret = phy_connect_direct(priv->netdev, priv->phydev,
				 &adin1140_handle_link_change,
				 PHY_INTERFACE_MODE_INTERNAL);
	if (ret)
		return dev_err_probe(&spidev->dev, ret,
				     "Can't attach PHY to %s\n",
				     priv->mdiobus->id);

	ret = devm_add_action_or_reset(&spidev->dev, adin1140_phy_remove,
				       priv->phydev);
	if (ret)
		return ret;

	phy_attached_info(priv->phydev);

	return 0;
}
/* …（同文件无关代码省略）… */
	if (ret)
		return ret;

	ret = adin1140_phy_init(priv, spi);
	if (ret)
		return ret;
/* …（同文件无关代码省略）… */

	netdev->if_port = IF_PORT_10BASET;
	netdev->irq = spi->irq;
	netdev->netdev_ops = &adin1140_netdev_ops;
	netdev->ethtool_ops = &adin1140_ethtool_ops;
	netdev->netns_immutable = true;
	netdev->priv_flags |= IFF_LIVE_ADDR_CHANGE |
