#pragma once

#include <stdbool.h>

#include <zephyr/device.h>

int can_printer_init_custom(const struct device *can_dev, bool has_start_msg, uint32_t start_msg_id,
			    bool has_stop_msg, uint32_t stop_msg_id, bool has_hello_msg,
			    uint32_t hello_msg_id);

int can_printer_init(const struct device *can_dev);

void can_printer_deinit(const struct device *can_dev);

bool can_printer_is_initialized(void);

bool can_printer_is_enabled(void);
