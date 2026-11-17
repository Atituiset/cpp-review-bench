// AUTO-DRAFT from torvalds/linux PR #141b164bbe186423e295b139e40e8e2d11cf1341
struct bonded_device {
	struct comedi_device *dev;
	unsigned int minor;
	unsigned int subdev;
	unsigned int nchans;
};
/* …（同文件无关代码省略）… */
		char file[sizeof("/dev/comediXXXXXX")];
		int minor = it->options[i];
		struct comedi_device *d;
		int sdev = -1, nchans;
		struct bonded_device *bdev;
		struct bonded_device **devs;
  // <<< BUG ANCHOR
/* …（同文件无关代码省略）… */
				dev_err(dev->class_dev,
					"comedi_get_n_channels() returned %d on minor %u subdev %d!\n",
					nchans, minor, sdev);
				return -EINVAL;
			}
			bdev = kmalloc_obj(*bdev);
			if (!bdev)
				return -ENOMEM;

			bdev->dev = d;
			bdev->minor = minor;
/* …（同文件无关代码省略）… */
				dev_err(dev->class_dev,
					"Could not allocate memory. Out of memory?\n");
				kfree(bdev);
				return -ENOMEM;
			}
			devpriv->devs = devs;
			devpriv->devs[devpriv->ndevs++] = bdev;

			/* Append dev:subdev to devpriv->name */
			seq_buf_printf(&s, "%u:%u ", bdev->minor, bdev->subdev);
		}
	}

	if (!devpriv->nchans) {
