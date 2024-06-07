/*
 * Copyright (c) 2024 Thorsten Spätling <thorsten.spaetling@vierling.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hw_revision

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <app/drivers/hw_revision.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hw_revision, CONFIG_HW_REVISION_LOG_LEVEL);

struct hw_revision_data {
	int state;
};

struct hw_revision_config {
	size_t num_inputs;
	const struct gpio_dt_spec *input;
};

static int hw_revision_sample_fetch(const struct device *dev,
				      enum sensor_channel chan)
{
	const struct hw_revision_config *config = dev->config;
	struct hw_revision_data *data = dev->data;
	int err = 0;

	if (!config->num_inputs)
	{
		LOG_ERR("%s: no gpios found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	data->state = 0;
	for (size_t i = 0; (i < config->num_inputs) && !err; i++)
	{
		const struct gpio_dt_spec *gpio = &config->input[i];
		data->state |= (gpio_pin_get_dt(gpio)<<i);
	}

	return err;
}

static int hw_revision_channel_get(const struct device *dev,
				     enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct hw_revision_data *data = dev->data;

	if (chan != SENSOR_CHAN_HW_REVISION) {
		return -ENOTSUP;
	}

	val->val1 = data->state;

	return 0;
}

static int hw_revision_init(const struct device *dev)
{
	const struct hw_revision_config *config = dev->config;

	int err = 0;

	if (!config->num_inputs)
	{
		LOG_ERR("%s: no gpios found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}
	for (size_t i = 0; (i < config->num_inputs) && !err; i++)
	{
		const struct gpio_dt_spec *gpio = &config->input[i];
		if (!device_is_ready(gpio->port)) {
			LOG_ERR("Input GPIO not ready");
			return -ENODEV;
		}
	}

	for (size_t i = 0; (i < config->num_inputs) && !err; i++)
	{
		const struct gpio_dt_spec *gpio = &config->input[i];
		err = gpio_pin_configure_dt(gpio, GPIO_INPUT | GPIO_PULL_UP);
		if (err < 0)
		{
			LOG_ERR("Input GPIO not ready");
			return -ENODEV;
		}
	}

	return err;
}

static const struct sensor_driver_api hw_revision_api = {
	.sample_fetch = &hw_revision_sample_fetch,
	.channel_get = &hw_revision_channel_get,
};

#define HW_REVISION_INIT(i)					\
								\
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {		\
	DT_INST_FOREACH_CHILD_SEP_VARGS(i, GPIO_DT_SPEC_GET, (,), input-gpios) \
};								\
								\
static struct hw_revision_data hw_revision_data_##i;	       \
static const struct hw_revision_config hw_revision_config_##i = {	\
	.num_inputs	= ARRAY_SIZE(gpio_dt_spec_##i),	\
	.input		= gpio_dt_spec_##i,			\
};								\
								\
DEVICE_DT_INST_DEFINE(i, &hw_revision_init, NULL,			\
		&hw_revision_data_##i, &hw_revision_config_##i,		\
		      POST_KERNEL, CONFIG_HW_REVISION_INIT_PRIORITY,	\
		      &hw_revision_api);

DT_INST_FOREACH_STATUS_OKAY(HW_REVISION_INIT)
