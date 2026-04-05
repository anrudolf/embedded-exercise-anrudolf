#pragma once

#include <stdint.h>

#include <zephyr/device.h>

int can_sender_init_custom(const struct device *dev, uint32_t msg_0_id, uint32_t msg_0_period_ms,
			   uint32_t msg_1_id, uint32_t msg_1_period_ms, uint32_t msg_2_id,
			   uint32_t msg_2_period_ms);

int can_sender_init(const struct device *can_dev);
