// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <linux/iio/iio.h>

#include "test_iio.h"

int test_iio_hw_read_channel(struct test_iio_state *st,
			     unsigned int channel,
			     int *value)
{
	switch (channel) {
	case 0:
		st->counter++;
		*value = st->counter;
		return 0;
	case 1:
		*value = -st->counter;
		return 0;
	default:
		return -EINVAL;
	}
}

int test_iio_hw_set_sampling_frequency(struct test_iio_state *st,
				       unsigned int frequency)
{
	st->sampling_frequency = frequency;
	return 0;
}

static int test_iio_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val,
			     int *val2,
			     long mask)
{
	struct test_iio_state *st = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&st->lock);
		ret = test_iio_hw_read_channel(st, chan->channel, val);
		mutex_unlock(&st->lock);
		if (ret)
			return ret;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		*val = 0;
		*val2 = 1000;
		return IIO_VAL_INT_PLUS_MICRO;

	case IIO_CHAN_INFO_SAMP_FREQ:
		*val = st->sampling_frequency;
		return IIO_VAL_INT;

	default:
		return -EINVAL;
	}
}

static int test_iio_write_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int val,
			      int val2,
			      long mask)
{
	struct test_iio_state *st = iio_priv(indio_dev);
	int ret;

	if (mask != IIO_CHAN_INFO_SAMP_FREQ)
		return -EINVAL;

	if (val <= 0)
		return -EINVAL;

	mutex_lock(&st->lock);
	ret = test_iio_hw_set_sampling_frequency(st, val);
	mutex_unlock(&st->lock);

	return ret;
}

static const struct iio_info test_iio_info = {
	.read_raw = test_iio_read_raw,
	.write_raw = test_iio_write_raw,
};

#define TEST_IIO_VOLTAGE_CHANNEL(_channel, _scan_index)      \
	{                                                       \
		.type = IIO_VOLTAGE,                              \
		.indexed = 1,                                     \
		.channel = (_channel),                            \
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),     \
		.info_mask_shared_by_type =                       \
			BIT(IIO_CHAN_INFO_SCALE) |                    \
			BIT(IIO_CHAN_INFO_SAMP_FREQ),                 \
		.scan_index = (_scan_index),                      \
		.scan_type = {                                    \
			.sign = 's',                                  \
			.realbits = TEST_IIO_REALBITS,                \
			.storagebits = TEST_IIO_STORAGE_BITS,          \
			.shift = 0,                                   \
			.endianness = IIO_LE,                         \
		},                                                \
	}

static const struct iio_chan_spec test_iio_channels[] = {
	TEST_IIO_VOLTAGE_CHANNEL(0, 0),
	TEST_IIO_VOLTAGE_CHANNEL(1, 1),
};

/*
 * V2 intentionally supports one scan layout only: CH0 + CH1.
 * This keeps the first buffer experiment simple and guarantees that the
 * struct test_iio_scan layout matches the active IIO scan layout.
 */
static const unsigned long test_iio_scan_masks[] = {
	BIT(0) | BIT(1),
	0,
};

static int test_iio_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct test_iio_state *st;
	int ret;

	dev_info(&pdev->dev, "probing V2 buffered test IIO device\n");

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);
	st->dev = &pdev->dev;
	st->indio_dev = indio_dev;
	mutex_init(&st->lock);
	st->counter = 0;
	st->sampling_frequency = TEST_IIO_DEFAULT_SAMPLING_FREQ;
	st->buffer_running = false;

	indio_dev->name = TEST_IIO_DEVICE_NAME;
	indio_dev->info = &test_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE | INDIO_BUFFER_SOFTWARE;
	indio_dev->channels = test_iio_channels;
	indio_dev->num_channels = ARRAY_SIZE(test_iio_channels);
	indio_dev->available_scan_masks = test_iio_scan_masks;

	ret = test_iio_buffer_setup(indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to set up IIO buffer: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, indio_dev);

	ret = devm_iio_device_register(&pdev->dev, indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register IIO device: %d\n", ret);
		test_iio_buffer_cleanup(indio_dev);
		return ret;
	}

	dev_info(&pdev->dev,
		 "registered V2 IIO device \"%s\" with software buffer\n",
		 indio_dev->name);

	return 0;
}

static int test_iio_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "removing V2 test IIO device\n");
	test_iio_buffer_cleanup(indio_dev);
	return 0;
}

static const struct of_device_id test_iio_of_match[] = {
	{ .compatible = "example,test-iio-adc" },
	{ }
};
MODULE_DEVICE_TABLE(of, test_iio_of_match);

static struct platform_driver test_iio_driver = {
	.probe = test_iio_probe,
	.remove = test_iio_remove,
	.driver = {
		.name = TEST_IIO_DRIVER_NAME,
		.of_match_table = test_iio_of_match,
	},
};

static bool autocreate = true;
module_param(autocreate, bool, 0444);
MODULE_PARM_DESC(autocreate, "Automatically create a virtual platform device");

static struct platform_device *test_iio_pdev;

static int __init test_iio_init(void)
{
	int ret;

	ret = platform_driver_register(&test_iio_driver);
	if (ret)
		return ret;

	if (!autocreate)
		return 0;

	test_iio_pdev = platform_device_register_simple(TEST_IIO_DRIVER_NAME,
						 -1, NULL, 0);
	if (IS_ERR(test_iio_pdev)) {
		ret = PTR_ERR(test_iio_pdev);
		platform_driver_unregister(&test_iio_driver);
		return ret;
	}

	return 0;
}

static void __exit test_iio_exit(void)
{
	if (autocreate && !IS_ERR_OR_NULL(test_iio_pdev))
		platform_device_unregister(test_iio_pdev);

	platform_driver_unregister(&test_iio_driver);
}

module_init(test_iio_init);
module_exit(test_iio_exit);

MODULE_AUTHOR("pengliu");
MODULE_DESCRIPTION("V2 IIO virtual ADC with software kfifo buffer");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
