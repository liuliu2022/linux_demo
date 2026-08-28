/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TEST_IIO_H
#define TEST_IIO_H

#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/workqueue.h>

struct iio_dev;

#define TEST_IIO_DRIVER_NAME              "test_iio"
#define TEST_IIO_DEVICE_NAME              "test-iio-adc"
#define TEST_IIO_NUM_CHANNELS             2
#define TEST_IIO_DEFAULT_SAMPLING_FREQ    1000
#define TEST_IIO_REALBITS                 16
#define TEST_IIO_STORAGEBITS              16

/* One V2 scan: CH0 followed by CH1. */
struct test_iio_scan {
	s16 ch0;
	s16 ch1;
};

struct test_iio_state {
	struct device *dev;
	struct iio_dev *indio_dev;
	struct mutex lock;

	/* Virtual ADC state. */
	s16 counter;
	unsigned int sampling_frequency;

	/* V2 software producer: hrtimer -> workqueue -> IIO buffer. */
	struct hrtimer sample_timer;
	struct work_struct sample_work;
	bool buffer_running;
};

int test_iio_hw_read_channel(struct test_iio_state *st,
			     unsigned int channel,
			     int *value);
int test_iio_hw_set_sampling_frequency(struct test_iio_state *st,
				       unsigned int frequency);

int test_iio_buffer_setup(struct iio_dev *indio_dev);
void test_iio_buffer_cleanup(struct iio_dev *indio_dev);

#endif /* TEST_IIO_H */
