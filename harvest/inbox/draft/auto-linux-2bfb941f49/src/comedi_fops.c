// AUTO-DRAFT from torvalds/linux PR #141b164bbe186423e295b139e40e8e2d11cf1341
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define COMEDI_SRF_RUNNING	BIT(27)
/* …（同文件无关代码省略）… */
static unsigned int __comedi_get_subdevice_runflags(struct comedi_subdevice *s)
{
	return s->runflags;
}
/* …（同文件无关代码省略）… */
static bool comedi_is_runflags_running(unsigned int runflags)
{
	return runflags & COMEDI_SRF_RUNNING;
}
/* …（同文件无关代码省略）… */
static bool __comedi_is_subdevice_running(struct comedi_subdevice *s)
{
	unsigned int runflags = __comedi_get_subdevice_runflags(s);

	return comedi_is_runflags_running(runflags);
}
/* …（同文件无关代码省略）… */
bool comedi_get_is_subdevice_running(struct comedi_subdevice *s)
{
	unsigned long flags;
	bool running;

	spin_lock_irqsave(&s->spin_lock, flags);
	running = __comedi_is_subdevice_running(s);
	if (running)
		refcount_inc(&s->async->run_active);
	spin_unlock_irqrestore(&s->spin_lock, flags);
	return running;
}
/* …（同文件无关代码省略）… */
void comedi_put_is_subdevice_running(struct comedi_subdevice *s)
{
	if (refcount_dec_and_test(&s->async->run_active))
		complete_all(&s->async->run_complete);
}
/* …（同文件无关代码省略）… */
void comedi_event(struct comedi_device *dev, struct comedi_subdevice *s)
{
	if (comedi_get_is_subdevice_running(s)) {
		comedi_event(dev, s);
		comedi_put_is_subdevice_running(s);
	}
}
