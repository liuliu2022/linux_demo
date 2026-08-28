// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/workqueue.h>

#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/kfifo_buf.h>

#include "test_iio.h"

static ktime_t test_iio_sample_period(struct test_iio_state *st)
{
	u64 ns;
	unsigned int frequency;

	frequency = READ_ONCE(st->sampling_frequency);
	if (!frequency)
		frequency = 1;

	ns = div_u64(NSEC_PER_SEC, frequency);
	if (!ns)
		ns = 1;

	return ns_to_ktime(ns);
}

static void test_iio_sample_work(struct work_struct *work)
{
	struct test_iio_state *st =
		container_of(work, struct test_iio_state, sample_work);
	struct test_iio_scan scan;
	int ret;

	if (!READ_ONCE(st->buffer_running))
		return;

	/*
	 * One software sampling event produces one complete scan:
	 *
	 *     [ CH0 ][ CH1 ]
	 *
	 * The two values belong to the same logical sampling instant.
	 */
	mutex_lock(&st->lock);
	st->counter++;
	scan.ch0 = st->counter;
	scan.ch1 = -st->counter;
	mutex_unlock(&st->lock);

	ret = iio_push_to_buffers(st->indio_dev, &scan);
	if (ret == -EBUSY)
		dev_warn_ratelimited(st->dev,
			"IIO buffer full, dropping scan\n");
	else if (ret)
		dev_warn_ratelimited(st->dev,
			"failed to push scan to IIO buffer: %d\n", ret);
}

static enum hrtimer_restart test_iio_sample_timer(struct hrtimer *timer)
{
	struct test_iio_state *st =
		container_of(timer, struct test_iio_state, sample_timer);

	if (!READ_ONCE(st->buffer_running))
		return HRTIMER_NORESTART;

	/*
	 * Keep the hrtimer callback short.  It only schedules process-context
	 * work; the work item builds the scan and calls iio_push_to_buffers().
	 */
	schedule_work(&st->sample_work);
	hrtimer_forward_now(timer, test_iio_sample_period(st));

	return HRTIMER_RESTART;
}

static int test_iio_buffer_postenable(struct iio_dev *indio_dev)
{
	struct test_iio_state *st = iio_priv(indio_dev);

	WRITE_ONCE(st->buffer_running, true);
	hrtimer_start(&st->sample_timer,
		      test_iio_sample_period(st),
		      HRTIMER_MODE_REL);

	dev_info(st->dev, "V2 software producer started\n");
	return 0;
}

static int test_iio_buffer_predisable(struct iio_dev *indio_dev)
{
	struct test_iio_state *st = iio_priv(indio_dev);

	WRITE_ONCE(st->buffer_running, false);
	hrtimer_cancel(&st->sample_timer);
	cancel_work_sync(&st->sample_work);

	dev_info(st->dev, "V2 software producer stopped\n");
	return 0;
}

static const struct iio_buffer_setup_ops test_iio_buffer_ops = {
	.postenable = test_iio_buffer_postenable,
	.predisable = test_iio_buffer_predisable,
};

int test_iio_buffer_setup(struct iio_dev *indio_dev)
{
	struct test_iio_state *st = iio_priv(indio_dev);
	struct iio_buffer *buffer;

	buffer = devm_iio_kfifo_allocate(st->dev);
	if (!buffer)
		return -ENOMEM;

	iio_device_attach_buffer(indio_dev, buffer);
	indio_dev->setup_ops = &test_iio_buffer_ops;

	INIT_WORK(&st->sample_work, test_iio_sample_work);
	hrtimer_init(&st->sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	st->sample_timer.function = test_iio_sample_timer;

	return 0;
}

void test_iio_buffer_cleanup(struct iio_dev *indio_dev)
{
	struct test_iio_state *st = iio_priv(indio_dev);

	WRITE_ONCE(st->buffer_running, false);
	hrtimer_cancel(&st->sample_timer);
	cancel_work_sync(&st->sample_work);
}
