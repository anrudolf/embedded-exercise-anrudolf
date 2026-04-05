#include <zephyr/ztest.h>
#include <zephyr/drivers/can.h>

#include "can_sender.h"

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

#define MAX_FRAMES 5

CAN_MSGQ_DEFINE(can_msgq_0, MAX_FRAMES);
CAN_MSGQ_DEFINE(can_msgq_1, MAX_FRAMES);
CAN_MSGQ_DEFINE(can_msgq_2, MAX_FRAMES);

#define NUM_THREADS   3
#define MY_STACK_SIZE 512
#define MY_PRIORITY   5

K_THREAD_STACK_ARRAY_DEFINE(my_stack_areas, NUM_THREADS, MY_STACK_SIZE);
struct k_thread my_thread_data[NUM_THREADS];

static void test_periodic_message(void *p_msgq, void *p_msg_id, void *p_msg_period_ms)
{
	struct k_msgq *msgq = (struct k_msgq *)p_msgq;
	uint32_t msg_id = (uint32_t)(uintptr_t)p_msg_id;
	uint32_t msg_period_ms = (uint32_t)(uintptr_t)p_msg_period_ms;

	struct can_frame frame_buffer;

	const struct can_filter filter = {.flags = 0U, .id = msg_id, .mask = CAN_STD_ID_MASK};

	int filter_id = can_add_rx_filter_msgq(can_dev, msgq, &filter);
	zassert_not_equal(filter_id, -ENOSPC, "no filters available");
	zassert_true(filter_id >= 0, "negative filter number");

	for (size_t i = 0; i < 10; i++) {
		int err = k_msgq_get(msgq, &frame_buffer, K_MSEC(msg_period_ms));
		zassert_ok(err, "receive timeout");
		zassert_equal(frame_buffer.id, msg_id, "unexpected message id, want %d, got %d",
			      msg_id, frame_buffer.id);
		zassert_equal(k_uptime_get(), msg_period_ms * (i + 1),
			      "unexpected message timestamp, want %d, got %d",
			      msg_period_ms * (i + 1), k_uptime_get());
		zassert_equal(k_msgq_num_used_get(msgq), 0,
			      "unexpected number of messages in queue, want 0, got %d",
			      k_msgq_num_used_get(msgq));
	}

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(can_sender_lib, test_periodic_messages)
{
	can_set_mode(can_dev, CAN_MODE_NORMAL | CAN_MODE_LOOPBACK);
	can_start(can_dev);
	can_sender_init(can_dev);

	k_tid_t tids[NUM_THREADS];

	struct k_msgq *can_msgqs[NUM_THREADS] = {&can_msgq_0, &can_msgq_1, &can_msgq_2};
	uint32_t msg_ids[NUM_THREADS] = {CONFIG_CAN_SENDER_TX_0_MSG_ID,
					 CONFIG_CAN_SENDER_TX_1_MSG_ID,
					 CONFIG_CAN_SENDER_TX_2_MSG_ID};
	uint32_t msg_periods_ms[NUM_THREADS] = {CONFIG_CAN_SENDER_TX_0_MSG_PERIOD_MS,
						CONFIG_CAN_SENDER_TX_1_MSG_PERIOD_MS,
						CONFIG_CAN_SENDER_TX_2_MSG_PERIOD_MS};

	for (size_t i = 0; i < NUM_THREADS; i++) {
		tids[i] = k_thread_create(
			&my_thread_data[i], my_stack_areas[i],
			K_THREAD_STACK_SIZEOF(my_stack_areas[i]), test_periodic_message,
			can_msgqs[i], (void *)(uintptr_t)msg_ids[i],
			(void *)(uintptr_t)msg_periods_ms[i], MY_PRIORITY, 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < NUM_THREADS; i++) {
		k_thread_join(tids[i], K_FOREVER);
	}

	can_stop(can_dev);
}

ZTEST_SUITE(can_sender_lib, NULL, NULL, NULL, NULL, NULL);
