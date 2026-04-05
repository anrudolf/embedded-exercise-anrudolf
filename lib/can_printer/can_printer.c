#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

#include "can_printer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_printer, CONFIG_CAN_PRINTER_LOG_LEVEL);

struct can_printer_cfg {
	bool initialized;
	bool enabled;
	int filter_id;
	bool has_start_msg;
	uint32_t start_msg_id;
	bool has_stop_msg;
	uint32_t stop_msg_id;
	bool has_hello_msg;
	uint32_t hello_msg_id;
} config;

/**
 * Print a CAN frame to console.
 *
 * This is pretty much a copy from Zephyr CAN shell's dump functionality.
 * Uses printk for output (but could be adapted to use LOG if desired).
 */
static void print_frame(struct can_frame *frame)
{
	uint8_t nbytes = can_dlc_to_bytes(frame->dlc);
	int i;

	printk("(%06d)  ", k_uptime_get());

	/* CAN ID */
	printk("%*s%0*x  ", (frame->flags & CAN_FRAME_IDE) != 0 ? 0 : 5, "",
	       (frame->flags & CAN_FRAME_IDE) != 0 ? 8 : 3,
	       (frame->flags & CAN_FRAME_IDE) != 0 ? frame->id & CAN_EXT_ID_MASK
						   : frame->id & CAN_STD_ID_MASK);

	/* DLC as number of bytes */
	printk("%s[%0*d]  ", (frame->flags & CAN_FRAME_FDF) != 0 ? "" : " ",
	       (frame->flags & CAN_FRAME_FDF) != 0 ? 2 : 1, nbytes);

	/* Data payload */
	if ((frame->flags & CAN_FRAME_RTR) != 0) {
		printk("remote transmission request");
	} else {
		for (i = 0; i < nbytes; i++) {
			printk("%02x ", frame->data[i]);
		}
	}

	printk("\n");
}

static void rx_callback_function(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct can_printer_cfg *cfg = (struct can_printer_cfg *)user_data;

	if (cfg->enabled) {
		print_frame(frame);
	}

	if (cfg->has_start_msg && frame->id == cfg->start_msg_id) {
		cfg->enabled = true;
	}

	if (cfg->has_stop_msg && frame->id == cfg->stop_msg_id) {
		cfg->enabled = false;
	}

	if (cfg->has_hello_msg && frame->id == cfg->hello_msg_id) {
		printk("hello specialized\n");
	}
}

bool can_printer_is_enabled(void)
{
	return config.enabled;
}

bool can_printer_is_initialized(void)
{
	return config.initialized;
}

void can_printer_deinit(const struct device *can_dev)
{
	can_remove_rx_filter(can_dev, config.filter_id);
	memset(&config, 0, sizeof(config));
}

int can_printer_init_custom(const struct device *can_dev, bool has_start_msg, uint32_t start_msg_id,
			    bool has_stop_msg, uint32_t stop_msg_id, bool has_hello_msg,
			    uint32_t hello_msg_id)
{
	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN device not ready");
		return -EIO;
	}

	const struct can_filter my_filter = {
		.flags = 0,
		.id = 0,
		.mask = 0 // no mask matches all messages
	};

	int filter_id = can_add_rx_filter(can_dev, rx_callback_function, &config, &my_filter);
	if (filter_id < 0) {
		LOG_ERR("Unable to add rx filter [%d]", filter_id);
		return filter_id;
	}

	config.initialized = true;
	config.enabled = !has_start_msg;
	config.filter_id = filter_id;

	config.has_start_msg = has_start_msg;
	config.start_msg_id = start_msg_id;
	config.has_stop_msg = has_stop_msg;
	config.stop_msg_id = stop_msg_id;
	config.has_hello_msg = has_hello_msg;
	config.hello_msg_id = hello_msg_id;

	return 0;
}

int can_printer_init(const struct device *can_dev)
{
	return can_printer_init_custom(
		can_dev,
		IS_ENABLED(CONFIG_CAN_PRINTER_START_MSG),
		COND_CODE_1(CONFIG_CAN_PRINTER_START_MSG, (CONFIG_CAN_PRINTER_START_MSG_ID), (0)),
		IS_ENABLED(CONFIG_CAN_PRINTER_STOP_MSG),
		COND_CODE_1(CONFIG_CAN_PRINTER_STOP_MSG, (CONFIG_CAN_PRINTER_STOP_MSG_ID), (0)),
		IS_ENABLED(CONFIG_CAN_PRINTER_HELLO_MSG),
		COND_CODE_1(CONFIG_CAN_PRINTER_HELLO_MSG, (CONFIG_CAN_PRINTER_HELLO_MSG_ID), (0))
	);
}
