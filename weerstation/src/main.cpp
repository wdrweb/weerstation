/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define TEST_GPIO_PIN 3

static int counter = 0;

int main(void)
{
#ifdef CONFIG_SAMPLE_DO_OUTPUT
	printk("Hello World from minimal!\n");
#endif

	const struct device *const gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio_dev)) {
		printk("GPIO device not ready\n");
		return 0;
	}

	gpio_pin_configure(gpio_dev, TEST_GPIO_PIN, GPIO_OUTPUT_INACTIVE);

	while (true) {
		gpio_pin_toggle(gpio_dev, TEST_GPIO_PIN);
		int pin_state = gpio_pin_get(gpio_dev, TEST_GPIO_PIN);
		printk("Counter: %d pin: %d\n", counter, pin_state);
		counter++;
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
