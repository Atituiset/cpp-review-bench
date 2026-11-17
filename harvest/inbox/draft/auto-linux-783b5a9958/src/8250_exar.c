// AUTO-DRAFT from torvalds/linux PR #b6793a9e2a3c280bb7c6c7d39be271e2f57aa2bb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct exar8250_board {
	unsigned int num_ports;
	unsigned int reg_shift;
	int	(*setup)(struct exar8250 *priv, struct pci_dev *pcidev,
			 struct uart_8250_port *port, int idx);
	void	(*exit)(struct pci_dev *pcidev);
};
/* …（同文件无关代码省略）… */
struct exar8250 {
	unsigned int		nr;
	unsigned int		osc_freq;
	struct exar8250_board	*board;
	struct eeprom_93cx6	eeprom;
	void __iomem		*virt;
	struct platform_device	*gpio;
	struct uart_8250_port	*uport[];
};
/* …（同文件无关代码省略）… */
static void __xr17v35x_unregister_gpio(struct platform_device *pdev)
{
	device_remove_software_node(&pdev->dev);
	platform_device_unregister(pdev);
}
/* …（同文件无关代码省略）… */

static void xr17v35x_unregister_gpio(struct exar8250 *priv)
{
	if (priv->gpio) {
		__xr17v35x_unregister_gpio(priv->gpio);
		priv->gpio = NULL;
	}
