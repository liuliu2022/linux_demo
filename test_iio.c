/*
 * test_iio.c
 *
 * Generic IIO driver template / virtual ADC
 *
 * Purpose:
 *   1. Learn and verify the Linux IIO subsystem.
 *   2. Be visible from libiio.
 *   3. Serve as a reusable skeleton for future ADC/IIO drivers.
 *
 * V1:
 *   - 2 voltage input channels
 *   - raw
 *   - scale
 *   - sampling_frequency
 *   - direct mode
 *   - scan description reserved for future buffered capture
 *
 * Future:
 *   V2 - triggered buffer
 *   V3 - hardware IRQ / timer
 *   V4 - DMA buffered capture
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mutex.h>

#include <linux/iio/iio.h>

/* ------------------------------------------------------------------------- */
/* Driver constants                                                          */
/* ------------------------------------------------------------------------- */

#define TEST_IIO_DRIVER_NAME       "test_iio"
#define TEST_IIO_DEVICE_NAME       "test-iio-adc"

#define TEST_IIO_NUM_CHANNELS      2

#define TEST_IIO_DEFAULT_SAMPLING_FREQ  1000

/*
 * Test ADC format:
 *
 * signed 16-bit
 * realbits    = 16
 * storagebits = 16
 */
#define TEST_IIO_REALBITS          16
#define TEST_IIO_STORAGEBITS       16


/* ------------------------------------------------------------------------- */
/* Private device state                                                      */
/* ------------------------------------------------------------------------- */

/*
 * This structure is private to one device instance.
 *
 * Later, when using real hardware, this is the main structure that you
 * extend with:
 *
 *     void __iomem *regs;
 *     struct clk *clk;
 *     int irq;
 *     DMA state;
 *     buffer state;
 *     hardware configuration;
 */
struct test_iio_state {
	struct device *dev;

	struct mutex lock;

	/*
	 * Fake ADC counter.
	 *
	 * CH0 returns counter
	 * CH1 returns -counter
	 */
	s16 counter;

	/*
	 * Device configuration.
	 */
	unsigned int sampling_frequency;
};


/* ------------------------------------------------------------------------- */
/* Hardware abstraction layer                                                */
/* ------------------------------------------------------------------------- */

/*
 * IMPORTANT:
 *
 * Keep hardware access separate from IIO callbacks.
 *
 * Today:
 *
 *     test_iio_hw_read_channel()
 *             |
 *             +--> generates fake data
 *
 * Future:
 *
 *     test_iio_hw_read_channel()
 *             |
 *             +--> readl(regs + ADC_DATA_REG)
 *
 * This allows the IIO-facing code to remain largely unchanged when
 * replacing the virtual ADC with real hardware.
 */

static int test_iio_hw_read_channel(struct test_iio_state *st,
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


/*
 * Stub for future real hardware configuration.
 */
static int test_iio_hw_set_sampling_frequency(struct test_iio_state *st,
					      unsigned int frequency)
{
	/*
	 * Future hardware implementation might do:
	 *
	 *     writel(divider, st->regs + SAMPLE_RATE_REG);
	 */

	st->sampling_frequency = frequency;

	return 0;
}


/* ------------------------------------------------------------------------- */
/* IIO callbacks                                                             */
/* ------------------------------------------------------------------------- */

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

		ret = test_iio_hw_read_channel(st,
					       chan->channel,
					       val);

		mutex_unlock(&st->lock);

		if (ret)
			return ret;

		return IIO_VAL_INT;


	case IIO_CHAN_INFO_SCALE:

		/*
		 * Fake ADC scale:
		 *
		 *     1 LSB = 1 mV
		 *
		 * IIO_VAL_INT_PLUS_MICRO:
		 *
		 *     val  = integer part
		 *     val2 = micro part
		 *
		 * Therefore:
		 *
		 *     0 + 1000 / 1,000,000
		 *       = 0.001
		 */
		*val  = 0;
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

	switch (mask) {

	case IIO_CHAN_INFO_SAMP_FREQ:

		if (val <= 0)
			return -EINVAL;

		mutex_lock(&st->lock);

		ret = test_iio_hw_set_sampling_frequency(st, val);

		mutex_unlock(&st->lock);

		return ret;


	default:
		return -EINVAL;
	}
}


/* ------------------------------------------------------------------------- */
/* IIO info                                                                  */
/* ------------------------------------------------------------------------- */

static const struct iio_info test_iio_info = {
	.read_raw  = test_iio_read_raw,
	.write_raw = test_iio_write_raw,
};


/* ------------------------------------------------------------------------- */
/* Channel description                                                       */
/* ------------------------------------------------------------------------- */

/*
 * Macro used so that adding more ADC channels later is easy.
 *
 * Example:
 *
 *     TEST_IIO_VOLTAGE_CHANNEL(0, 0)
 *     TEST_IIO_VOLTAGE_CHANNEL(1, 1)
 *     TEST_IIO_VOLTAGE_CHANNEL(2, 2)
 */

#define TEST_IIO_VOLTAGE_CHANNEL(_channel, _scan_index)		\
	{							\
		.type = IIO_VOLTAGE,				\
		.indexed = 1,					\
		.channel = (_channel),				\
								\
		.info_mask_separate =				\
			BIT(IIO_CHAN_INFO_RAW),			\
								\
		.info_mask_shared_by_type =			\
			BIT(IIO_CHAN_INFO_SCALE) |		\
			BIT(IIO_CHAN_INFO_SAMP_FREQ),		\
								\
		.scan_index = (_scan_index),			\
								\
		.scan_type = {					\
			.sign = 's',				\
			.realbits = TEST_IIO_REALBITS,		\
			.storagebits = TEST_IIO_STORAGEBITS,	\
			.shift = 0,				\
			.endianness = IIO_LE,			\
		},						\
	}


static const struct iio_chan_spec test_iio_channels[] = {

	TEST_IIO_VOLTAGE_CHANNEL(0, 0),

	TEST_IIO_VOLTAGE_CHANNEL(1, 1),

	/*
	 * We will add timestamp in the buffered version.
	 *
	 * IIO_CHAN_SOFT_TIMESTAMP(2),
	 */
};


/* ------------------------------------------------------------------------- */
/* Probe / Remove                                                            */
/* ------------------------------------------------------------------------- */

static int test_iio_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct test_iio_state *st;
	int ret;

	dev_info(&pdev->dev, "probing generic test IIO device\n");


	/*
	 * Allocate:
	 *
	 *     struct iio_dev
	 *
	 * together with:
	 *
	 *     struct test_iio_state
	 *
	 * Private state is accessed using:
	 *
	 *     iio_priv(indio_dev)
	 */
	indio_dev = devm_iio_device_alloc(&pdev->dev,
					 sizeof(*st));

	if (!indio_dev)
		return -ENOMEM;


	st = iio_priv(indio_dev);

	st->dev = &pdev->dev;

	mutex_init(&st->lock);

	st->counter = 0;

	st->sampling_frequency =
		TEST_IIO_DEFAULT_SAMPLING_FREQ;


	/*
	 * Describe this IIO device.
	 */
	indio_dev->name = TEST_IIO_DEVICE_NAME;

	indio_dev->info = &test_iio_info;

	indio_dev->modes = INDIO_DIRECT_MODE;

	indio_dev->channels = test_iio_channels;

	indio_dev->num_channels =
		ARRAY_SIZE(test_iio_channels);


	/*
	 * Save IIO device pointer as platform driver data.
	 */
	platform_set_drvdata(pdev, indio_dev);


	/*
	 * Register with the Linux IIO core.
	 */
	ret = devm_iio_device_register(&pdev->dev,
				       indio_dev);

	if (ret) {
		dev_err(&pdev->dev,
			"failed to register IIO device: %d\n",
			ret);

		return ret;
	}


	dev_info(&pdev->dev,
		 "registered IIO device \"%s\"\n",
		 indio_dev->name);

	return 0;
}


static int test_iio_remove(struct platform_device *pdev)
{
	/*
	 * devm_iio_device_register()
	 * and
	 * devm_iio_device_alloc()
	 *
	 * are device-managed resources.
	 *
	 * Therefore they are automatically cleaned up.
	 */

	dev_info(&pdev->dev,
		 "removing test IIO device\n");

	return 0;
}


/* ------------------------------------------------------------------------- */
/* Device Tree matching                                                      */
/* ------------------------------------------------------------------------- */

/*
 * Future device tree:
 *
 * test_adc@0 {
 *     compatible = "example,test-iio-adc";
 * };
 *
 * For this V1 virtual device we do not actually need a DT node because the
 * module can optionally create its own platform device below.
 */

static const struct of_device_id test_iio_of_match[] = {
	{
		.compatible = "example,test-iio-adc",
	},
	{ }
};

MODULE_DEVICE_TABLE(of, test_iio_of_match);


/* ------------------------------------------------------------------------- */
/* Platform driver                                                           */
/* ------------------------------------------------------------------------- */

static struct platform_driver test_iio_driver = {

	.probe  = test_iio_probe,
	.remove = test_iio_remove,

	.driver = {
		.name           = TEST_IIO_DRIVER_NAME,
		.of_match_table = test_iio_of_match,
	},
};


/* ------------------------------------------------------------------------- */
/* Auto-created test platform device                                         */
/* ------------------------------------------------------------------------- */

/*
 * Why is this here?
 *
 * A normal production driver obtains its platform_device from Device Tree.
 *
 * But this is a TEST TEMPLATE.
 *
 * We want:
 *
 *     insmod test_iio.ko
 *
 * to work immediately without first editing the device tree.
 *
 * Later, on real hardware:
 *
 *     autocreate=0
 *
 * and the device will come from Device Tree.
 */

static bool autocreate = true;

module_param(autocreate, bool, 0444);

MODULE_PARM_DESC(autocreate,
		 "Automatically create a virtual platform device");


static struct platform_device *test_iio_pdev;


/* ------------------------------------------------------------------------- */
/* Module init / exit                                                        */
/* ------------------------------------------------------------------------- */

static int __init test_iio_init(void)
{
	int ret;

	ret = platform_driver_register(&test_iio_driver);

	if (ret)
		return ret;


	if (!autocreate)
		return 0;


	test_iio_pdev =
		platform_device_register_simple(
			TEST_IIO_DRIVER_NAME,
			-1,
			NULL,
			0);

	if (IS_ERR(test_iio_pdev)) {

		ret = PTR_ERR(test_iio_pdev);

		platform_driver_unregister(
			&test_iio_driver);

		return ret;
	}


	return 0;
}


static void __exit test_iio_exit(void)
{
	if (autocreate &&
	    !IS_ERR_OR_NULL(test_iio_pdev))
		platform_device_unregister(
			test_iio_pdev);


	platform_driver_unregister(
		&test_iio_driver);
}


module_init(test_iio_init);
module_exit(test_iio_exit);


/* ------------------------------------------------------------------------- */
/* Module information                                                        */
/* ------------------------------------------------------------------------- */

MODULE_AUTHOR("pengliu");
MODULE_DESCRIPTION("Generic reusable IIO test ADC driver template");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
