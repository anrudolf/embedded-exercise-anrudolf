#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>

#include "can_printer.h"
#include "can_sender.h"

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static void send_empty_frame(uint32_t msg_id)
{
	struct can_frame frame = {0};

	frame.id = msg_id;
	frame.dlc = 0;

	can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
}

/**
 *  This test sends a start message after one second and a stop message after another second,
 *  folllowed by a hello message.
 *
 *  A console harness will capture the output and verify that CAN frames are printed between the
 *  start and stop messages only. The hello message is always printed.
 */
int main(void)
{
	can_set_mode(can_dev, CAN_MODE_NORMAL | CAN_MODE_LOOPBACK);
	can_start(can_dev);

	can_printer_init(can_dev);
	can_sender_init(can_dev);

	printk("TEST_BEGIN_MARKER\n");

	k_sleep(K_SECONDS(1));
	send_empty_frame(CONFIG_CAN_PRINTER_START_MSG_ID);

	k_sleep(K_SECONDS(1));
	send_empty_frame(CONFIG_CAN_PRINTER_STOP_MSG_ID);

	k_sleep(K_SECONDS(1));
	send_empty_frame(CONFIG_CAN_PRINTER_HELLO_MSG_ID);

	k_sleep(K_SECONDS(1));
	printk("TEST_END_MARKER\n");

	return 0;
}
