#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_CAN_PRINTER)
#include "can_printer.h"
#endif

#if defined(CONFIG_CAN_SENDER)
#include "can_sender.h"
#endif

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

int main(void)
{
	int err;

#if defined(CONFIG_APP_CAN_LOOPBACK)
	err = can_set_mode(can_dev, CAN_MODE_NORMAL | CAN_MODE_LOOPBACK);
	if (err < 0) {
		LOG_ERR("Error setting CAN into LOOPBACK mode (err %d)", err);
		return err;
	}
#endif

	err = can_start(can_dev);
	if (err < 0) {
		LOG_ERR("Error starting CAN device (err %d)", err);
		return err;
	}

#if defined(CONFIG_CAN_PRINTER)
	err = can_printer_init(can_dev);
	if (err < 0) {
		LOG_ERR("Error initializing CAN printer (err %d)", err);
		return err;
	}
#endif

#if defined(CONFIG_CAN_SENDER)
	err = can_sender_init(can_dev);
	if (err < 0) {
		LOG_ERR("Error initializing CAN sender (err %d)", err);
		return err;
	}
#endif

	return 0;
}
