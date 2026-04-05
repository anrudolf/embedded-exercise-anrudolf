#include <zephyr/ztest.h>
#include <zephyr/drivers/can.h>

#include "can_printer.h"

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#define TEST_SEND_TIMEOUT K_MSEC(100)

#define TEST_START_MSG_ID 1
#define TEST_STOP_MSG_ID  2
#define TEST_HELLO_MSG_ID 3

static void send_empty_frame(uint32_t msg_id)
{
	struct can_frame frame = {0};

	frame.id = msg_id;
	frame.dlc = 0;

	can_send(can_dev, &frame, TEST_SEND_TIMEOUT, NULL, NULL);

	k_sleep(K_MSEC(100));
}

ZTEST(can_printer_lib, test_is_enabled)
{
	can_printer_init_custom(can_dev, true, TEST_START_MSG_ID, true, TEST_STOP_MSG_ID, true,
				TEST_HELLO_MSG_ID);
	zassert_false(can_printer_is_enabled(),
		      "should be disabled after initialization with start message");

	send_empty_frame(TEST_START_MSG_ID);
	zassert_true(can_printer_is_enabled(), "should be enabled after receiving start message");

	send_empty_frame(TEST_STOP_MSG_ID);
	zassert_false(can_printer_is_enabled(), "should be disabled after receiving stop message");
}

ZTEST(can_printer_lib, test_no_start_msg)
{
	can_printer_init_custom(can_dev, false, 0, true, TEST_STOP_MSG_ID, true, TEST_HELLO_MSG_ID);
	zassert_true(can_printer_is_enabled(),
		     "should be enabled when initialized without start message");
}

ZTEST(can_printer_lib, test_no_stop_msg)
{
	can_printer_init_custom(can_dev, false, 0, false, 0, true, TEST_HELLO_MSG_ID);
	zassert_true(can_printer_is_enabled(),
		     "should be enabled when initialized without start message");

	send_empty_frame(TEST_STOP_MSG_ID);
	zassert_true(can_printer_is_enabled(),
		     "should remain enabled when stop message is not configured");
}

static void test_setup(void *fixture)
{
	int err = can_set_mode(can_dev, CAN_MODE_NORMAL | CAN_MODE_LOOPBACK);
	zassert_equal(err, 0, "Failed to set CAN mode (err %d)", err);
	err = can_start(can_dev);
	zassert_equal(err, 0, "Failed to start CAN device (err %d)", err);
}

static void test_cleanup(void *fixture)
{
	can_printer_deinit(can_dev);
	can_stop(can_dev);
}

ZTEST_SUITE(can_printer_lib, NULL, NULL, test_setup, test_cleanup, NULL);
